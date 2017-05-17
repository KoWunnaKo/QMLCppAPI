/****************************************************************************
** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
****************************************************************************/
{% set comma = joiner(",") %}

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QTextStream>

#include "model/Model.h"
#include "model/JSON.h"

#include "property/Property.h"

// Dependencies
{% for field in struct.fields %}
{{field|requiredInclude}}
{% endfor %}

{{module|namespaceOpen}}

class {{struct.name}} : public TModelStructure<
{% for field in struct.fields %}
    {{ comma() }}
    {{field|returnType}}
{% endfor %}
>
{
    Q_GADGET

public:

    static const FieldNames FIELD_NAMES;

    Q_PROPERTY(int id READ id CONSTANT)   // This seems to be necessary even if the base class already contains an "id" property. TODO : clarify

    {{struct.name}}() :
    {%-for field in struct.fields %}
        m_{{field}}(std::get<{{loop.index-1}}>(m_values))
        {% if not loop.last %},{% endif %}
    {% endfor %}
    {
    }

    {{struct.name}}(const {{struct.name}}& other) :
    {%-for field in struct.fields %}
        m_{{field}}(std::get<{{loop.index-1}}>(m_values))
        {% if not loop.last %},{% endif %}
    {% endfor %}
    {
        copyFrom(other);
    }

    {{struct.name}}& operator=(const {{struct.name}} &right) {
        copyFrom(right);
        return *this;
    }

    static QHash<int, QByteArray> roleNames() {
        return roleNames_(FIELD_NAMES);
    }

    Q_INVOKABLE {{struct|fullyQualifiedCppName}} clone() const {
        {{struct.name}} s;
        s.setValue(asTuple());
        return s;
    }

    QString toString() const {
        return toStringWithFields(FIELD_NAMES);
    }

{% for field in struct.fields %}
    Q_PROPERTY({{field|returnType}} {{field}} READ {{field}} WRITE set{{field}})

    {{field|returnType}} {{field.name}}() const {
        return m_{{field.name}};
    }

    void set{{field.name}}({{field|returnType}} value) {
    	qDebug() << "Setting field {{field.name}} with value:"; // << value;
        m_{{field.name}} = value;
    }

{% endfor %}

public:
{% for field in struct.fields %}
    {{field|returnType}}& m_{{field}};
{% endfor %}

};





class {{struct.name}}ListProperty : public StructListProperty<{{struct.name}}> {

    Q_OBJECT

public:

    Q_INVOKABLE void addElement({{struct | fullyQualifiedCppName}} element) {
        StructListProperty<{{struct.name}}>::addElement(element);
    }

    Q_INVOKABLE {{struct | fullyQualifiedCppName}} elementById(int elementId) const {
        return StructListProperty<{{struct.name}}>::elementById(elementId);
    }

    Q_INVOKABLE int elementIndexById(int elementId) const {
        return StructListProperty<{{struct.name}}>::elementIndexById(elementId);
    }

    Q_INVOKABLE {{struct.name}} elementAt(int index) const {
        return StructListProperty<{{struct.name}}>::elementAt(index);
    }

};

{{module|namespaceClose}}


template<> inline void writeJSON(QJsonValue& json, const {{struct|fullyQualifiedCppName}}& value) {
    QJsonObject subObject;
    Q_UNUSED(value);
    {% for field in struct.fields %}
    {
        QJsonValue v;
        writeJSON(v, value.m_{{field}});
        subObject["{{field}}"] = v;
    }
    {% endfor %}
    json = subObject;
}

template<> inline void readJSON(const QJsonValue& json, {{struct|fullyQualifiedCppName}}& value) {
    QJsonObject subObject = json.toObject();
    Q_UNUSED(value);
    {% for field in struct.fields %}
    readJSON(subObject["{{field}}"], value.m_{{field}});
    {% endfor %}
}


inline QTextStream &operator <<(QTextStream &outStream, const {{struct|fullyQualifiedCppName}}& f) {
    outStream << f.toString();
    return outStream;
}

Q_DECLARE_METATYPE(QList<{{struct|fullyQualifiedCppName}}>)   // Needed for list properties
Q_DECLARE_METATYPE({{struct|fullyQualifiedCppName}})


inline QJSValue toJSValue(const {{struct|fullyQualifiedCppName}}& f) {
    return structToJSValue(f);
}

