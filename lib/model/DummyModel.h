/*
 *   This is part of the QMLCppAPI project
 *   Copyright (C) 2017 Pelagicore AB
 *   SPDX-License-Identifier: LGPL-2.1
 *   This file is subject to the terms of the LGPL 2.1 license.
 *   Please see the LICENSE file for details. 
 */

#pragma once

#include <QMainWindow>
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QPalette>
#include <QDir>
#include <QComboBox>
#include <QScrollArea>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "JSON.h"

#include "Model.h"
#include "property/Property.h"


class PropertyWidget : public QWidget {
public:

    typedef std::function<void()> ChangeListener;

    PropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : QWidget(parent) {
        m_propertyName = propertyName;

        m_layout = new QHBoxLayout();
        setLayout(m_layout);

        m_propertyNameLabel = new QLabel();
        m_layout->addWidget(m_propertyNameLabel);
        setPropertyName(propertyName);
        setAutoFillBackground(true);
    }

    void setWidget(QWidget* widget) {
        m_layout->addWidget(widget);
    }

    void setPropertyName(const QString& propertyName) {
        m_propertyNameLabel->setText(propertyName);
    }

    const QString& propertyName() const {
        return m_propertyName;
    }

    void setListener(ChangeListener listener) {
        m_listener= listener;
    }

    QHBoxLayout *m_layout;
    QLabel *m_propertyNameLabel;
    ChangeListener m_listener;
    QString m_propertyName;
};

template <typename EnumType>
class EnumerationPropertyWidget : public PropertyWidget {

public:
    EnumerationPropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : PropertyWidget(propertyName, parent) {
        widget = new QComboBox();
        setWidget(widget);
        auto values = validValues<EnumType>();
        for(auto& v:values) {
            widget->addItem(toString(v), static_cast<int>(v));
        }
    }

    EnumType value() const {
        int index = widget->currentIndex();
        return validValues<EnumType>()[index];
    }

    void init(EnumType initialValue) {

        auto values = validValues<EnumType>();
        for(int i = 0; i < values.size(); i++) {
            if (initialValue == values[i])
            widget->setCurrentIndex(i);
        }

        QObject::connect(widget, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int) {
            m_listener();
        });

    }

    QComboBox* widget = nullptr;
};


class BooleanPropertyWidget : public PropertyWidget {

public:
    BooleanPropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : PropertyWidget(propertyName, parent) {
        widget = new QCheckBox();
        setWidget(widget);
    }

    bool value() const {
        return widget->isChecked();
    }

    void init(bool initialValue) {
        widget->setChecked(initialValue);
        QObject::connect(widget, &QCheckBox::stateChanged, this, [this]() {
            m_listener();
        });
    }

    QCheckBox* widget = nullptr;
};


class IntegerPropertyWidget : public PropertyWidget {

public:
    IntegerPropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : PropertyWidget(propertyName, parent) {
        widget = new QSpinBox();
        setWidget(widget);
    }

    int value() const {
        return widget->value();
    }

    void init(int initialValue) {
        widget->setValue(initialValue);
        QObject::connect(widget, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
            m_listener();
        });
    }

    QSpinBox* widget = nullptr;
};


class StringPropertyWidget : public PropertyWidget {

public:
    StringPropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : PropertyWidget(propertyName, parent) {
        widget = new QTextEdit();
        setWidget(widget);
    }

    QString value() const {
        return widget->toPlainText();
    }

    void init(const QString& initialValue) {
        widget->setText(initialValue);
        QObject::connect(widget, &QTextEdit::textChanged, [this]() {
            m_listener();
        });
    }

    QTextEdit* widget = nullptr;
};


template<typename ElementType>
class ListPropertyWidget : public PropertyWidget {

public:
    ListPropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : PropertyWidget(propertyName, parent) {
        createNewElementButton = new QPushButton("Create new element");
        setWidget(createNewElementButton);
    }

    void setValueWidget(QWidget* widget) {
        m_layout->addWidget(widget);
    }

    QPushButton* createNewElementButton;

};


template<typename ElementType>
class SimpleListPropertyWidget : public ListPropertyWidget<ElementType> {

public:
    SimpleListPropertyWidget(const QString& propertyName, QWidget* parent = nullptr) : ListPropertyWidget<ElementType>(propertyName, parent) {
    }

    void init(const QList<ElementType>& initialValue) {
//        Q_ASSERT(false);
        Q_UNUSED(initialValue);
    }

    QList<ElementType> value() const {
        Q_ASSERT(false);
        return QList<ElementType>();
    }

};

template <typename Type, typename Sfinae = void> struct DummyUIDesc {
    typedef PropertyWidget PanelType;
};

template <typename ListElementType> struct DummyUIDesc<QList<ListElementType>> {
    typedef SimpleListPropertyWidget<ListElementType> PanelType;
};

template <> struct DummyUIDesc<bool> {
    typedef BooleanPropertyWidget PanelType;
};

template <> struct DummyUIDesc<int> {
    typedef IntegerPropertyWidget PanelType;
};

template <> struct DummyUIDesc<float> {
    typedef IntegerPropertyWidget PanelType;
};

template <> struct DummyUIDesc<QString> {
    typedef StringPropertyWidget PanelType;
};

template<typename StructType>
class StructurePropertyWidget : public PropertyWidget {

public:
    StructurePropertyWidget(const QString& propertyName, QWidget* parent = nullptr) :
        PropertyWidget(propertyName, parent) {
        auto widget = new QWidget();
        m_layout = new QVBoxLayout();
        widget->setLayout(m_layout);
        setWidget(widget);
    }

    template<std::size_t I = 0, typename ... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    create_widget_panel(std::tuple<Tp...>& t)
    {
        Q_UNUSED(t);
    }

    template<std::size_t I = 0, typename ... Tp>
    inline typename std::enable_if<I < sizeof...(Tp), void>::type
    create_widget_panel(std::tuple<Tp...>& t)
    {
        createPanelForField(std::get<I>(t), StructType::FIELD_NAMES[I]);
        create_widget_panel<I + 1, Tp...>(t);
    }

    template<typename FieldType> void createPanelForField(FieldType& v, const char* fieldName) {

        typedef typename DummyUIDesc<FieldType>::PanelType PanelType;
        auto widget = new PanelType(fieldName);

        widget->init(v);
        widget->setListener([this, widget, &v ]() {
            v = widget->value();
            if(m_listener)
                m_listener();
        });

        if (widget != nullptr) {
            m_layout->addWidget(widget);
        }
        Q_UNUSED(v);
    }

    void init(const StructType& initialValue = StructType()) {
        m_fieldValues = initialValue.asTuple();
        create_widget_panel(m_fieldValues);
    }

    void add(PropertyWidget* child) {
        m_layout->addWidget(child);
    }

    const StructType& value() {
        m_value.setValue(m_fieldValues);
        return m_value;
    }

    typename StructType::FieldTupleTypes m_fieldValues;
    StructType m_value;

    QVBoxLayout* m_layout;
};


template <typename StructType>
struct DummyUIDesc<StructType, typename std::enable_if< std::is_base_of< ModelStructure, StructType >::value>::type> {
    typedef StructurePropertyWidget<StructType> PanelType;
};

template <typename Type> inline void assignRandomValue(Type& t) {
    t = {};
}

#include <random>
template <> inline void assignRandomValue(int& t) {
    static std::normal_distribution<double> normal_dist(653, 10);
    typedef std::mt19937 MyRNG;  // the Mersenne Twister with a popular choice of parameters
    MyRNG rng;
    t = normal_dist(rng);
}

template <> inline void assignRandomValue(QString& t) {
    t = "Random string";
}

template<std::size_t I = 0, typename ... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
random_tuple(std::tuple<Tp...>& t)
{
    Q_UNUSED(t);
}

template<std::size_t I = 0, typename ... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
random_tuple(std::tuple<Tp...>& t)
{
    assignRandomValue(std::get<I>(t));
    random_tuple<I + 1, Tp...>(t);
}

template <typename ListElementType> inline QWidget* createWidget(StructListProperty<ListElementType>& t, const QString& propertyName) {
    auto widget = new ListPropertyWidget<ListElementType>(propertyName);

    QObject::connect(widget->createNewElementButton, &QPushButton::clicked, [&t]() {
        typename ListElementType::FieldTupleTypes values;
        random_tuple(values);
        t.addElement(ListElementType(values));
    });

    return widget;
}

template <typename ElementType>
inline QTextStream &operator <<(QTextStream &outStream, const QList<ElementType>& f) {
    Q_UNUSED(outStream);
    Q_UNUSED(f);
//    outStream << toString(f);
    return outStream;
}


template <typename Type, typename Sfinae = void> struct ToStringDesc {
    static void appendToString(QTextStream& s, const Type& value) {
        s << value;
    }

};


class DummyModelBase : public QObject {

    Q_OBJECT

protected:
    DummyModelBase(QObject* parent) : QObject(parent) {
    }

    virtual ~DummyModelBase();

    void init();

    QWidget* m_window;
    QVBoxLayout *m_layout;
    QLabel* m_logLabel;
    bool m_autoSaveEnabled = true;

    QList<PropertyWidget*> m_widgets;

    bool m_oddWidget = true;

    QPushButton* m_saveSnapshotButton;
    QPushButton* m_loadSnapshotButton;
    QPushButton* m_clearLogButton;
    QCheckBox* m_autoSaveCheckBox;

};

/**
 * Abstract class for dummy models
 */
template <typename TypeName>
class DummyModel : public DummyModelBase {

public:
    DummyModel(QObject* parent = nullptr) : DummyModelBase(parent) {
    }

    void init() {

        DummyModelBase::init();
        m_window->setWindowTitle(TypeName::INTERFACE_NAME);

        loadSettings();
        if (m_autoSaveEnabled) {
            loadJSONSnapshot();
        }

        m_autoSaveCheckBox->setChecked(m_autoSaveEnabled);

        QObject::connect(m_saveSnapshotButton, &QPushButton::clicked, [this]() {
            saveJSONSnapshot();
        });
        QObject::connect(m_loadSnapshotButton, &QPushButton::clicked, [this]() {
            loadJSONSnapshot();
        });
        QObject::connect(m_clearLogButton, &QPushButton::clicked, [this]() {
            m_logLabel->setText("");
        });

        QObject::connect(m_autoSaveCheckBox, &QCheckBox::stateChanged, [this]() {
            m_autoSaveEnabled = m_autoSaveCheckBox->isChecked();
            saveSettings();
        });

    }

    void addWidget(PropertyWidget& widget) {
        QPalette pal;
        m_oddWidget = !m_oddWidget;
        pal.setColor(QPalette::Background, m_oddWidget ? Qt::lightGray : Qt::gray);
        widget.setPalette(pal);
        m_layout->addWidget(&widget);
        m_widgets.append(&widget);
    }

    QString getModelPersistenceFolder() const {
        auto path = QDir::currentPath() + QStringLiteral("/models/");
        return path;
    }

    QString getJSONSnapshotFilePath() const {
        QString path = getModelPersistenceFolder() + TypeName::INTERFACE_NAME + ".snapshot";
        return path;
    }

    QString getSettingsFilePath() const {
        QString path = getModelPersistenceFolder() + TypeName::INTERFACE_NAME + ".settings";
        return path;
    }

    virtual void writeJsonValues(QJsonObject& jsonObject) const = 0;

    virtual void loadJsonValues(const QJsonObject& jsonObject) = 0;

    void writeToFile(const QJsonObject& jsonObject, const QString& filePath) const {
        QJsonDocument doc(jsonObject);
        QFile jsonFile(filePath);
        QDir folder = QFileInfo(filePath).absoluteDir();
        if (!folder.mkpath( folder.path() ))
            qWarning() << "Can't create folder " << folder;

        auto success = jsonFile.open(QFile::WriteOnly);
        if (success) {
            jsonFile.write(doc.toJson());
            qDebug() << "JSON snapshot written to " << jsonFile.fileName() << " : " << doc.toJson();
        }
        else
            qWarning() << "Can't save JSON snapshot to file " << jsonFile.fileName();
    }

    void saveJSONSnapshot() const {
        QJsonObject jsonObject;
        writeJsonValues(jsonObject);
        writeToFile(jsonObject, getJSONSnapshotFilePath());
    }

    void loadJSONSnapshot() {
        bool success;
        auto jsonDoc = loadJSONFile(getJSONSnapshotFilePath(), success);
        if(success) {
            loadJsonValues(jsonDoc.object());
        }
    }

    QJsonDocument loadJSONFile(const QString& filePath, bool& success) {
        QFile jsonFile(filePath);
        success = jsonFile.open(QFile::ReadOnly);
        if(success) {
            QJsonDocument jsonDoc = QJsonDocument().fromJson(jsonFile.readAll());
            qDebug() << "Loaded JSON file " << jsonFile.fileName();
            return jsonDoc;
        }
        else
            qWarning() << "Can't load JSON snapshot to file " << jsonFile.fileName();

        return QJsonDocument();
    }

    static constexpr const char* AUTOSAVE_JSON_FIELD = "autosave";

    void loadSettings() {
        bool success;
        auto settingsDoc = loadJSONFile(getSettingsFilePath(), success);
        if (success) {
            auto settingsObject = settingsDoc.object();
            m_autoSaveEnabled = settingsObject[AUTOSAVE_JSON_FIELD].toBool();
        }
    }

    void saveSettings() const {
        QJsonObject settings;
        settings[AUTOSAVE_JSON_FIELD] = m_autoSaveEnabled;
        writeToFile(settings, getSettingsFilePath());
    }

    template<typename ElementType> void writeJSONProperty(QJsonObject& json, const Property<ElementType>& property, const char* propertyName) const {
        QJsonValue jsonValue;
        writeJSON(jsonValue, property.value());
        json[propertyName] = jsonValue;
    }

    template<typename ListElementType> void writeJSONProperty(QJsonObject& json, const StructListProperty<ListElementType>& property, const char* propertyName) const {
        QJsonArray array;

        for (auto& propertyElement : property.list()) {
            QJsonValue jsonValue;
            writeJSON(jsonValue, propertyElement);
            array.append(jsonValue);
        }

        json[propertyName] = array;
    }

    template<typename ListElementType> void writeJSONProperty(QJsonObject& json, const SimpleTypeListProperty<ListElementType>& property, const char* propertyName) const {
        QJsonArray array;

        for (auto& propertyElement : property.list()) {
            QJsonValue jsonValue;
            writeJSON(jsonValue, propertyElement);
            array.append(jsonValue);
        }

        json[propertyName] = array;
    }

    template<typename ElementType> void readJSONProperty(const QJsonObject& json, Property<ElementType>& property, const char* propertyName) const {
        ElementType v = {};
        readJSON(json[propertyName], v);
        property = v;
    }

    template<typename ListElementType> void readJSONProperty(const QJsonObject& json, StructListProperty<ListElementType>& property, const char* propertyName) const {

        if (json[propertyName].isArray()) {
            QList<ListElementType> elements;
            auto jsonArray = json[propertyName].toArray();
            auto size = jsonArray.size();

            for(int i = 0 ; i < size ; i++) {
                ListElementType e;
                readJSON(jsonArray[i], e);
                elements.append(e);
            }

            property.setList(elements);
        }
        else {
            qWarning() << "Expected array in property " << propertyName;
        }

    }

    template<typename ListElementType> void readJSONProperty(const QJsonObject& json, SimpleTypeListProperty<ListElementType>& property, const char* propertyName) const {

        if (json[propertyName].isArray()) {
            QList<ListElementType> elements;
            auto jsonArray = json[propertyName].toArray();
            auto size = jsonArray.size();

            for(int i = 0 ; i < size ; i++) {
                ListElementType e = {};
                readJSON(jsonArray[i], e);
                elements.append(e);
            }

            property.setList(elements);
        }
        else {
            qWarning() << "Expected array in property " << propertyName;
        }

    }



    template<typename PropertyType> void initWidget(Property<PropertyType>& property, const QString& propertyName) {

        Q_UNUSED(property);

        typedef typename DummyUIDesc<PropertyType>::PanelType PanelType;
        auto widget = new PanelType(propertyName);

        widget->init(property.value());
        widget->setListener([&property, widget]() {
            property = widget->value();
        });

        connect(property.owner(), property.signal(), this, [this]() {
            if (m_autoSaveEnabled) {
                saveJSONSnapshot();
            }
        });

        addWidget(*widget);
    }

    template<typename ListElementType> void initWidget(StructListProperty<ListElementType>& property, const QString& propertyName) {

        auto widget = new ListPropertyWidget<ListElementType>(propertyName);

        typedef typename DummyUIDesc<ListElementType>::PanelType ElementPanelType;
        auto widgetForNewElement = new ElementPanelType("New");
        widgetForNewElement->init();
        widget->setValueWidget(widgetForNewElement);

        QObject::connect(widget->createNewElementButton, &QPushButton::clicked, [&property, widgetForNewElement]() {
            property.addElement(widgetForNewElement->value().clone());
        });

        addWidget(*widget);

        connect(property.owner(), property.signal(), this, [this]() {
            if (m_autoSaveEnabled) {
                saveJSONSnapshot();
            }
        });

    }

    template<typename ListElementType> void initWidget(SimpleTypeListProperty<ListElementType>& property, const QString& propertyName) {

//        Q_ASSERT(false);
        qWarning () << "TODO : implement";
        auto widget = new ListPropertyWidget<ListElementType>(propertyName);

        typedef typename DummyUIDesc<ListElementType>::PanelType ElementPanelType;
        auto widgetForNewElement = new ElementPanelType("New");
//        widgetForNewElement->init();
        widget->setValueWidget(widgetForNewElement);

//        QObject::connect(widget->createNewElementButton, &QPushButton::clicked, [&property, widgetForNewElement]() {
//            property.addElement(widgetForNewElement->value().clone());
//        });

        addWidget(*widget);

        connect(property.m_ownerObject, property.m_ownerSignal, this, [this]() {
            if (m_autoSaveEnabled) {
                saveJSONSnapshot();
            }
        });
    }

    void generateToString(QTextStream& message) {
        Q_UNUSED(message);
    }

    template<typename FirstParameterTypes, typename ... ParameterTypes>
    void generateToString(QTextStream& message, const FirstParameterTypes& firstParameter, const ParameterTypes & ... parameters) {
        ToStringDesc<FirstParameterTypes>::appendToString(message, firstParameter);
        generateToString(message, parameters ...);
    }

    template<typename ... ParameterTypes>
    void logMethodCall(const QString methodName, const ParameterTypes & ... parameters) {
        QString argString;
        QTextStream s(&argString);
        generateToString(s, parameters ...);

        appendLog(methodName + " called with args : " + argString);
    }

    void appendLog(QString textToAppend) {
        QString text = m_logLabel->text() + "\n" + textToAppend;
        m_logLabel->setText(text);
    }

    template<typename ParameterType>
    void logSetterCall(const QString propertyName, const ParameterType & value) {
        appendLog(propertyName + " setter called with value : " + toString(value));
    }

};

