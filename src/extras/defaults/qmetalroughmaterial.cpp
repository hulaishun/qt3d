/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmetalroughmaterial.h"
#include "qmetalroughmaterial_p.h"
#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtexture.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <QUrl>
#include <QVector3D>
#include <QVector4D>

QT_BEGIN_NAMESPACE

using namespace Qt3DRender;

namespace Qt3DExtras {

QMetalRoughMaterialPrivate::QMetalRoughMaterialPrivate()
    : QMaterialPrivate()
    , m_environmentIrradianceTexture(new QTexture2D())
    , m_environmentSpecularTexture(new QTexture2D())
    , m_baseColorParameter(new QParameter(QStringLiteral("baseColor"), QColor("grey")))
    , m_metallicParameter(new QParameter(QStringLiteral("metallic"), 0.0f))
    , m_roughnessParameter(new QParameter(QStringLiteral("roughness"), 0.0f))
    , m_environmentIrradianceParameter(new QParameter(QStringLiteral("skyIrradiance"), m_environmentIrradianceTexture))
    , m_environmentSpecularParameter(new QParameter(QStringLiteral("skySpecular"), m_environmentSpecularTexture))
    , m_metalRoughEffect(new QEffect())
    , m_metalRoughGL3Technique(new QTechnique())
    , m_metalRoughGL3RenderPass(new QRenderPass())
    , m_metalRoughGL3Shader(new QShaderProgram())
    , m_filterKey(new QFilterKey)
{
    m_environmentIrradianceTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_environmentIrradianceTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_environmentIrradianceTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_environmentIrradianceTexture->setGenerateMipMaps(true);
    m_environmentIrradianceTexture->setMaximumAnisotropy(16.0f);

    m_environmentSpecularTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_environmentSpecularTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_environmentSpecularTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_environmentSpecularTexture->setGenerateMipMaps(true);
    m_environmentSpecularTexture->setMaximumAnisotropy(16.0f);
}

void QMetalRoughMaterialPrivate::init()
{
    connect(m_baseColorParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMetalRoughMaterialPrivate::handleBaseColorChanged);
    connect(m_metallicParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMetalRoughMaterialPrivate::handleMetallicChanged);
    connect(m_roughnessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMetalRoughMaterialPrivate::handleRoughnessChanged);
    connect(m_environmentIrradianceParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMetalRoughMaterialPrivate::handleEnvironmentIrradianceChanged);
    connect(m_environmentSpecularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMetalRoughMaterialPrivate::handleEnvironmentSpecularChanged);

    m_metalRoughGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/metalrough.vert"))));
    m_metalRoughGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/metalroughuniform.frag"))));

    m_metalRoughGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_metalRoughGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_metalRoughGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_metalRoughGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    Q_Q(QMetalRoughMaterial);
    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_metalRoughGL3Technique->addFilterKey(m_filterKey);
    m_metalRoughGL3RenderPass->setShaderProgram(m_metalRoughGL3Shader);
    m_metalRoughGL3Technique->addRenderPass(m_metalRoughGL3RenderPass);
    m_metalRoughEffect->addTechnique(m_metalRoughGL3Technique);

    m_metalRoughEffect->addParameter(m_baseColorParameter);
    m_metalRoughEffect->addParameter(m_metallicParameter);
    m_metalRoughEffect->addParameter(m_roughnessParameter);
    m_metalRoughEffect->addParameter(m_environmentIrradianceParameter);
    m_metalRoughEffect->addParameter(m_environmentSpecularParameter);

    q->setEffect(m_metalRoughEffect);
}

void QMetalRoughMaterialPrivate::handleBaseColorChanged(const QVariant &var)
{
    Q_Q(QMetalRoughMaterial);
    emit q->baseColorChanged(var.value<QColor>());
}

void QMetalRoughMaterialPrivate::handleMetallicChanged(const QVariant &var)
{
    Q_Q(QMetalRoughMaterial);
    emit q->metallicChanged(var.toFloat());
}
void QMetalRoughMaterialPrivate::handleRoughnessChanged(const QVariant &var)
{
    Q_Q(QMetalRoughMaterial);
    emit q->roughnessChanged(var.toFloat());
}

void QMetalRoughMaterialPrivate::handleEnvironmentIrradianceChanged(const QVariant &var)
{
    Q_Q(QMetalRoughMaterial);
    emit q->environmentIrradianceChanged(var.value<QAbstractTexture *>());
}

void QMetalRoughMaterialPrivate::handleEnvironmentSpecularChanged(const QVariant &var)
{
    Q_Q(QMetalRoughMaterial);
    emit q->environmentSpecularChanged(var.value<QAbstractTexture *>());
}

/*!
    \class Qt3DExtras::QMetalRoughMaterial
    \brief The QMetalRoughMaterial provides a default implementation of PBR
    lighting.
    \inmodule Qt3DExtras
    \since 5.9
    \inherits Qt3DRender::QMaterial

    This material uses an effect with a single render pass approach and performs per fragment
    lighting. Techniques are provided for OpenGL 3 only.
*/

/*!
    Constructs a new QMetalRoughMaterial instance with parent object \a parent.
*/
QMetalRoughMaterial::QMetalRoughMaterial(QNode *parent)
    : QMaterial(*new QMetalRoughMaterialPrivate, parent)
{
    Q_D(QMetalRoughMaterial);
    d->init();
}

/*! \internal */
QMetalRoughMaterial::QMetalRoughMaterial(QMetalRoughMaterialPrivate &dd, QNode *parent)
    : QMaterial(dd, parent)
{
    Q_D(QMetalRoughMaterial);
    d->init();
}

/*!
    Destroys the QMetalRoughMaterial instance.
*/
QMetalRoughMaterial::~QMetalRoughMaterial()
{
}

/*!
    \property QMetalRoughMaterial::baseColor

    Holds the current base color of the material.
*/
QColor QMetalRoughMaterial::baseColor() const
{
    Q_D(const QMetalRoughMaterial);
    return d->m_baseColorParameter->value().value<QColor>();
}

/*!
    \property QMetalRoughMaterial::metallic

    Holds the current metallic level of the material, since is a value between 0 (purely dielectric, the default)
    and 1 (purely metallic).
*/
float QMetalRoughMaterial::metallic() const
{
    Q_D(const QMetalRoughMaterial);
    return d->m_metallicParameter->value().toFloat();
}

/*!
    \property QMetalRoughMaterial::roughness

    Holds the current roughness level of the material.
*/
float QMetalRoughMaterial::roughness() const
{
    Q_D(const QMetalRoughMaterial);
    return d->m_roughnessParameter->value().toFloat();
}

/*!
    \property QMetalRoughMaterial::environmentIrradiance

    Holds the current environment irradiance map texture.

    By default, the environment irradiance texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QMetalRoughMaterial::environmentIrradiance() const
{
    Q_D(const QMetalRoughMaterial);
    return d->m_environmentIrradianceParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QMetalRoughMaterial::environmentSpecular

    Holds the current environment specular map texture.

    By default, the environment specular texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QMetalRoughMaterial::environmentSpecular() const
{
    Q_D(const QMetalRoughMaterial);
    return d->m_environmentSpecularParameter->value().value<QAbstractTexture *>();
}

void QMetalRoughMaterial::setBaseColor(const QColor &baseColor)
{
    Q_D(QMetalRoughMaterial);
    d->m_baseColorParameter->setValue(QVariant::fromValue(baseColor));
}

void QMetalRoughMaterial::setMetallic(float metallic)
{
    Q_D(QMetalRoughMaterial);
    d->m_metallicParameter->setValue(QVariant::fromValue(metallic));
}

void QMetalRoughMaterial::setRoughness(float roughness)
{
    Q_D(QMetalRoughMaterial);
    d->m_roughnessParameter->setValue(QVariant::fromValue(roughness));
}

void QMetalRoughMaterial::setEnvironmentIrradiance(QAbstractTexture *environmentIrradiance)
{
    Q_D(QMetalRoughMaterial);
    d->m_environmentIrradianceParameter->setValue(QVariant::fromValue(environmentIrradiance));
}

void QMetalRoughMaterial::setEnvironmentSpecular(QAbstractTexture *environmentSpecular)
{
    Q_D(QMetalRoughMaterial);
    d->m_environmentSpecularParameter->setValue(QVariant::fromValue(environmentSpecular));
}

} // namespace Qt3DExtras

QT_END_NAMESPACE
