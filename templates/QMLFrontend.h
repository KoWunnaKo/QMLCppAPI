{% set class = '{0}'.format(interface) %}
{% set comma = joiner(",") %}

/****************************************************************************

** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
****************************************************************************/

#pragma once

#include <QtCore>

#include "{{module|upperfirst}}Module.h"
#include "model/qmlfrontend.h"

// Dependencies
{% for property in interface.properties %}
{{property|requiredInclude}}
{% endfor %}

{% for operation in interface.operations %}
{% for parameter in operation.parameters %}
{{parameter|requiredInclude}}
{% endfor %}
{% endfor %}

{% for event in interface.events %}
{% for parameter in event.parameters %}
{{parameter|requiredInclude}}
{% endfor %}
{% endfor %}


{{module|namespaceOpen}}

/**
 * Definition of the {{interface}} interface.interface
 */
class {{class}}QMLFrontend : public QMLFrontend {

    Q_OBJECT

public:

    static constexpr const char* INTERFACE_NAME = "{{interface}}";
    static constexpr const char* IPC_INTERFACE_NAME = "{{interface|fullyQualifiedName|lower}}";

    {{class}}QMLFrontend(QObject* parent = nullptr) :
        QMLFrontend(parent) {
    }

    void init({{class}}& provider) {
        m_provider = &provider;
        {% for property in interface.properties %}
        connect(m_provider, &{{class}}::{{property.name}}Changed, this, &{{class}}QMLFrontend::{{property.name}}Changed);
        {% endfor %}

        {% for event in interface.events %}
        connect(m_provider, &{{class}}::{{event.name}}, this, &{{class}}QMLFrontend::{{event.name}});
        {% endfor %}

    }

    {% for property in interface.properties %}

    {% if property.type.is_model -%}

    Q_PROPERTY(QObject* {{property}} READ {{property}}_ NOTIFY {{property.name}}Changed);
    ModelListModel* {{property}}_() {return &m_provider->{{property}}();}

    {% elif property.type.is_list -%}
    Q_PROPERTY(QList<QVariant> {{property}} READ {{property}} NOTIFY {{property.name}}Changed);   // Exposing QList<ActualType> to QML does not seem to work
    QList<QVariant> {{property}}() const {
        return toQMLCompatibleType(m_provider->{{property}}());
    }

/*
    virtual {{property|returnType}} {{property}}() const {
        return m_provider->{{property}}();
    }
*/

    {% else %}
    Q_PROPERTY({{property|returnType}} {{property}} READ {{property}} NOTIFY {{property.name}}Changed);
    virtual {{property|returnType}} {{property}}() {
        return m_provider->{{property}}();
    }
    {% endif %}

    Q_SIGNAL void {{property}}Changed();

    {% endfor %}

    {% for operation in interface.operations %}
    Q_INVOKABLE virtual {{operation|returnType}} {{operation}}(
        {% set comma = joiner(",") %}
        {% for parameter in operation.parameters %}
        {{ comma() }}
        {{parameter|returnType}} {{parameter.name}}
        {% endfor %}
    ) {
        m_provider->{{operation}}(
                {% set comma = joiner(",") %}
                {% for parameter in operation.parameters %}
                {{ comma() }}
                {{parameter.name}}
                {% endfor %}
                );

    }
    {% endfor %}


    {% for event in interface.events %}
    Q_SIGNAL void {{event}}(
        {% set comma = joiner(",") %}
        {% for parameter in event.parameters %}
        {{ comma() }}
        {{parameter|returnType}} {{parameter.name}}
        {% endfor %}
    );
    {% endfor %}

    {{class}}* m_provider;
};


{{module|namespaceClose}}

