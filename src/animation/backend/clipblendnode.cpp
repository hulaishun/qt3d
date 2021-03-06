/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "clipblendnode_p.h"
#include <Qt3DAnimation/qclipblendnodecreatedchange.h>
#include <Qt3DAnimation/qanimationclip.h>
#include <Qt3DCore/qpropertynoderemovedchange.h>
#include <Qt3DCore/qpropertynodeaddedchange.h>

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {

namespace Animation {

ClipBlendNode::ClipBlendNode(BlendType blendType)
    : BackendNode(ReadOnly) // Makes sense for now at least
    , m_manager(nullptr)
    , m_blendType(blendType)
{
}

ClipBlendNode::~ClipBlendNode()
{
}

Qt3DCore::QNodeId ClipBlendNode::parentId() const
{
    return m_parentId;
}

Qt3DCore::QNodeIdVector ClipBlendNode::childrenIds() const
{
    return m_childrenIds;
}

Qt3DCore::QNodeIdVector ClipBlendNode::clipIds() const
{
    return m_clipIds;
}

void ClipBlendNode::sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e)
{
    switch (e->type()) {

    case Qt3DCore::PropertyValueAdded: {
       Qt3DCore::QPropertyNodeAddedChangePtr change = qSharedPointerCast<Qt3DCore::QPropertyNodeAddedChange>(e);
        if (change->metaObject()->inherits(&QAbstractClipBlendNode::staticMetaObject))
            addChildId(change->addedNodeId());
        else if (change->metaObject()->inherits(&QAnimationClip::staticMetaObject))
            m_clipIds.push_back(change->addedNodeId());
        break;
    }

    case Qt3DCore::PropertyValueRemoved: {
        Qt3DCore::QPropertyNodeRemovedChangePtr change = qSharedPointerCast<Qt3DCore::QPropertyNodeRemovedChange>(e);
        if (change->metaObject()->inherits(&QAbstractClipBlendNode::staticMetaObject))
            removeChildId(change->removedNodeId());
        else if (change->metaObject()->inherits(&QAnimationClip::staticMetaObject))
            m_clipIds.removeOne(change->removedNodeId());
        break;
    }

    default:
        break;
    }

    Qt3DCore::QBackendNode::sceneChangeEvent(e);
}

void ClipBlendNode::setClipBlendNodeManager(ClipBlendNodeManager *manager)
{
    m_manager = manager;
}

ClipBlendNodeManager *ClipBlendNode::clipBlendNodeManager() const
{
    return m_manager;
}

void ClipBlendNode::initializeFromPeer(const Qt3DCore::QNodeCreatedChangeBasePtr &change)
{
    const auto creationChange = qSharedPointerCast<QClipBlendNodeCreatedChangeBase>(change);
    setParentId(creationChange->parentClipBlendNodeId());
    m_clipIds = creationChange->clips();
}

void ClipBlendNode::setParentId(Qt3DCore::QNodeId parentId)
{
    if (parentId != m_parentId) {
        // We already had a parent, tell it to abandon us
        if (!m_parentId.isNull()) {
            ClipBlendNode *parent = m_manager->lookupNode(m_parentId);
            if (parent != nullptr)
                parent->m_childrenIds.removeAll(peerId());
        }
        m_parentId = parentId;
        ClipBlendNode *parent = m_manager->lookupNode(m_parentId);
        if (parent != nullptr && !parent->m_childrenIds.contains(peerId()))
            parent->m_childrenIds.append(peerId());
    }
}

void ClipBlendNode::addChildId(Qt3DCore::QNodeId childId)
{
    if (!m_childrenIds.contains(childId)) {
        ClipBlendNode *child = m_manager->lookupNode(childId);
        if (child != nullptr) {
            m_childrenIds.push_back(childId);
            child->m_parentId = peerId();
        }
    }
}

void ClipBlendNode::removeChildId(Qt3DCore::QNodeId childId)
{
    if (m_childrenIds.contains(childId)) {
        ClipBlendNode *child = m_manager->lookupNode(childId);
        if (child != nullptr)
            child->m_parentId = Qt3DCore::QNodeId();
        m_childrenIds.removeAll(childId);
    }
}

ClipBlendNode::BlendType Animation::ClipBlendNode::blendType() const
{
    return m_blendType;
}

} // Animation

} // Qt3DAnimation

QT_END_NAMESPACE
