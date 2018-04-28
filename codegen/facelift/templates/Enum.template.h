/****************************************************************************
** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
****************************************************************************/

#pragma once

#include <QtCore>

#include "FaceliftModel.h"

{{module|namespaceOpen}}

class {{enum}}Gadget
{
    Q_GADGET
public:
    enum Type {
        {% set comma = joiner(",") %}
        {%- for member in enum.members -%}
        {{ comma() }}
        {{member.name}} = {{member.value}}
        {%- endfor %}
    };
    Q_ENUM(Type)

};

typedef {{enum}}Gadget::Type {{enum}};

{{module|namespaceClose}}

Q_DECLARE_METATYPE({{enum|fullyQualifiedCppName}}Gadget::Type)

/*
template <> inline QVariant toVariant(const {{enum|fullyQualifiedCppName}}& v) {
    return static_cast<int>(v);
}
*/


namespace facelift {

template<> inline const QList<{{enum|fullyQualifiedCppName}}>& validValues<{{enum|fullyQualifiedCppName}}>() {
	static QList<{{enum|fullyQualifiedCppName}}> values = {
	{% for member in enum.members %}
	{{enum|fullyQualifiedCppName}}::{{member}},
	{% endfor %}
	};
	return values;
}

template <> inline QString enumToString(const {{enum|fullyQualifiedCppName}}& v) {
    const char* s = "Invalid";
    switch(v) {
    {% for member in enum.members %}
    case {{enum|fullyQualifiedCppName}}::{{member}}:
        s = "{{member}}";
        break;
    {% endfor %}
        default:
            break;
    }
    return s;
}

}

inline void assignFromString(const QString &s, {{enum|fullyQualifiedCppName}}& v)
{
    {% for member in enum.members %}
	if (s == "{{member}}")
		v = {{enum|fullyQualifiedCppName}}::{{member}};
    else
    {% endfor %}
	qFatal("No enum value matching string");
}


inline QTextStream &operator <<(QTextStream &outStream, const {{enum|fullyQualifiedCppName}}& f) {
    outStream << facelift::toString(f);
    return outStream;
}
