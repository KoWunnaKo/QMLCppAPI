/*
 *   This is part of the QMLCppAPI project
 *   Copyright (C) 2017 Pelagicore AB
 *   SPDX-License-Identifier: LGPL-2.1
 *   This file is subject to the terms of the LGPL 2.1 license.
 *   Please see the LICENSE file for details. 
 */

#pragma once

#include <vector>
#include <assert.h>
#include <QString>
#include <QObject>
#include <QDebug>
#include <QTimer>
#include <functional>

#include <model/Model.h>


class PropertyBase {

public:

    typedef ModelInterface BaseObject;
    typedef void (BaseObject::*ChangeSignal)();

    PropertyBase() {
    }

    void doTriggerChangeSignal() {
        (m_ownerObject->*m_ownerSignal)();
        clean();
    }

    void onValueChanged() {
        if (!m_timerEnabled) {
            m_timerEnabled = true;

            QTimer::singleShot(0, m_ownerObject, [this] () {
                if(isValueChanged()) {
                    qDebug() << "Triggering notification. value : " << toString();
                    doTriggerChangeSignal();
                }
                else {
                    qDebug() << "Triggering notification. value unchanged: " << toString();
                }
                m_timerEnabled = false;
            });

        }
    }

    template <typename ServiceType>
    void init(BaseObject* ownerObject, void (ServiceType::*changeSignal)()) {
        m_ownerObject = ownerObject;
        m_ownerSignal = (ChangeSignal) changeSignal;
    }

    virtual QString toString() const = 0;

    virtual bool isValueChanged() const = 0;

    virtual void clean() = 0;

    BaseObject* owner() {
        return m_ownerObject;
    }

    ChangeSignal signal() {
        return m_ownerSignal;
    }

private:
    BaseObject* m_ownerObject = nullptr;
    ChangeSignal m_ownerSignal = nullptr;

    bool m_timerEnabled = false;

};


template<typename Type>
class Property : public PropertyBase {

    typedef Type (BaseObject::*GetterMethod)();

    typedef std::function<Type()> GetterLambda;

public:

/*
    template <typename ServiceType>
    Property(BaseObject* ownerObject, void (ServiceType::*changeSignal)()) {
        init(ownerObject, changeSignal);
    }
*/

    void breakBinding() {
        if (m_boundObject != nullptr) {
            qWarning() << "Breaking binding";
            m_boundObject = nullptr;
            auto successfull = QObject::disconnect(m_connection);
            Q_ASSERT(successfull);
        }
    }

    template <typename BoundType> void bind(BoundType& obj, Type (BoundType::*getter)(), void (BoundType::*changeSignal)()) {
        breakBinding();
        m_boundObject = &obj;
        m_getter = (GetterMethod) getter;
        m_connection = QObject::connect(&obj, changeSignal, m_ownerObject, m_ownerSignal);
    }

public:

    QString toString() const override {
        QString result;
        QTextStream(&result) << "value = " << value();
        return result;
    }

    const Type value() const {
        if (m_lambda) {
            return m_lambda();
        } else if (m_boundObject != nullptr)
            return (m_boundObject->*m_getter)();
        else
            return m_value;
    }

    Type& value() {
        // We return a modifiable reference so we might have to trigger a "value changed" signal later
        onValueChanged();
        return m_value;
    }

    operator const Type() const {
        return value();
    }

    void setValueLambda(const GetterLambda& f) {
        breakBinding();
        m_lambda = f;
        onValueChanged();
    }

    void setValue(const Type &right) {
        breakBinding();
        if (!(m_value == right)) {
            m_value = right;
            onValueChanged();
        }
    }

    Type& operator=(const Type &right) {
        setValue(right);
        return m_value;
    }

    Type* operator->() {
        return &value();
    }

    bool isValueChanged() const override {
        return !(m_previousValue == value());
    }

    void clean() override {
        m_previousValue = m_value;
    }

    Type operator -=(const Type &right) {
        return operator=(value() - right);
    }

    Type operator +=(const Type &right) {
        return operator=(value() + right);
    }

    Type operator *=(const Type &right) {
        return operator=(value() * right);
    }

    Type operator /=(const Type &right) {
        return operator=(value() / right);
    }

private:
    Type m_value = { };
    Type m_previousValue = m_value;

    BaseObject* m_boundObject = nullptr;
    GetterMethod m_getter = nullptr;

    GetterLambda m_lambda;

    QMetaObject::Connection m_connection;

};


class ListPropertyBase : public QObject, public PropertyBase {

    Q_OBJECT

public:
    ListPropertyBase() {
    }

    Q_INVOKABLE virtual int size() const = 0;
    Q_INVOKABLE virtual void clear() = 0;

    bool isValueChanged() const override {
        return true;
    }

    QString toString() const override {
        qWarning() << "TODO";
        return "";
    }

    void clean() override {
        qWarning() << "TODO";
    }

};


class StructListPropertyBase : public ListPropertyBase {

    Q_OBJECT

public:
    StructListPropertyBase() {
    }

    Q_INVOKABLE virtual int addElement() = 0;
    Q_INVOKABLE virtual bool elementExists(int elementId) const = 0;

};


template<typename ElementType> class StructListProperty : public StructListPropertyBase {

    class TheModelListModel : public ModelListModel {

    public:

        TheModelListModel(StructListProperty& listProperty, const QList<ElementType>& list) : m_listProperty(listProperty), m_list(list) {
        }

        QHash<int,QByteArray> roleNames() const override {
            return ElementType::roleNames_(ElementType::FIELD_NAMES);
        }

        int rowCount(const QModelIndex& index) const override {
            Q_UNUSED(index);
            return m_list.size();
        }

        QVariant data(const QModelIndex& index, int role) const override {
            return m_list.at(index.row()).getFieldAsVariant(role);
        }

        void beginChange() {
            if(!m_changeOnGoing) {

                m_changeOnGoing = true;
                ModelListModel::beginResetModel();

                QTimer::singleShot(0, this, [this] () {
                    ModelListModel::endResetModel();
                    m_changeOnGoing = false;
                    m_listProperty.doTriggerChangeSignal();
                });

            }
        }

        int elementID(int elementIndex) const override {
            Q_ASSERT(elementIndex>=0);
            Q_ASSERT(elementIndex<m_list.size());

            if (!(elementIndex>=0) && (elementIndex<m_list.size())) {
                qWarning() << "Invalid index : " << elementIndex;
                return -1;
            }

            auto& element = m_list[elementIndex];
            return element.id();
        }

    private:
        StructListProperty& m_listProperty;
        const QList<ElementType>& m_list;
    };

public:

    StructListProperty() : m_model(*this, m_list) {
    }

    /**
     * Returns the model object exposed to QML
     */
    ModelListModel& getModel() {
        return m_model;
    }

    // TODO : remove need for that method
    QList<ElementType>& value() {
        return list();
    }

    /**
     * Returns a modifiable instance of the list.
     */
    QList<ElementType>& list() {
        m_model.beginChange();
        return m_list;
    }

    void setList(const QList<ElementType>& newList) {
        list() = newList;
    }

    const QList<ElementType>& list() const {
        return m_list;
    }

    int addElement() override {
        ElementType newElement;
        addElement(newElement);
        return newElement.id();
    }

    void addElement(ElementType element) {
        list().append(element);
    }

    int size() const override {
        return m_list.size();
    }

    bool elementExists(ModelElementID elementId) const override {
        for(const auto& element : m_list) {
            if(element.id() == elementId)
                return true;
        }
        return false;
    }

    ElementType elementById(ModelElementID elementId) const {
        Q_ASSERT(elementExists(elementId));
        for(const auto& element : m_list) {
            if(element.id() == elementId)
                return element;
        }
        return ElementType();
    }

    int elementIndexById(ModelElementID elementId) const {
        for (int index=0;index<m_list.size();index++) {
            if (m_list[index].id() == elementId)
                return index;
        }
        return -1;
    }

    void setValue(const QList<ElementType>& other) {
        m_list = other;
        onValueChanged();
    }

    operator const QList<ElementType>&() const {
        return m_list;
    }

    operator const QList<QVariant>() const {
        QList<QVariant> s;
        for(const auto& e : m_list)
            s.append(QVariant::fromValue(e));
        return s;
    }

    void clear() override {
        list().clear();
    }

    ElementType elementAt(int index) const {
        return m_list[index];
    }

    ElementType* elementPointerById(ModelElementID elementId) {
        Q_ASSERT(elementExists(elementId));
        for(auto& element : m_list) {
            if(element.id() == elementId)
                return &element;
        }
        return nullptr;
    }

    const ElementType* elementPointerById(ModelElementID id) const {
        for(const auto& element : m_list) {
            if(element.id() == id)
                return &element;
        }
        return nullptr;
    }

private:
    QList<ElementType> m_list;
    TheModelListModel m_model;

};

template<typename ElementType>
class SimpleTypeListProperty: public ListPropertyBase {

    class TheModelListModel : public ModelListModel {

    public:

        TheModelListModel(SimpleTypeListProperty& listProperty, const QList<ElementType>& list) : m_listProperty(listProperty), m_list(list) {
        }

        QHash<int,QByteArray> roleNames() const override {
            QHash<int,QByteArray> roles;
            roles[1000] = "data";
            return roles;
        }

        int rowCount(const QModelIndex& index) const override {
            Q_UNUSED(index);
            return m_list.size();
        }

        QVariant data(const QModelIndex& index, int role) const override {
            Q_UNUSED(role);
            Q_UNUSED(index);
            Q_ASSERT(false);
            return 1;
        }

        void beginChange() {
            if(!m_changeOnGoing) {

                m_changeOnGoing = true;
                ModelListModel::beginResetModel();

                QTimer::singleShot(0, this, [this] () {
                    ModelListModel::endResetModel();
                    m_changeOnGoing = false;

                    emit m_listProperty.valueChanged();
                });

            }
        }

        int elementID(int elementIndex) const override {
            Q_ASSERT(elementIndex>=0);
            Q_ASSERT(elementIndex<m_list.size());
            return 0;
        }

    private:
        SimpleTypeListProperty& m_listProperty;
        const QList<ElementType>& m_list;
    };

public:
    SimpleTypeListProperty() : m_model(*this, m_list) {
    }

    int size() const {
        return m_list.size();
    }

    void clear() {
    }

    /**
     * Returns a modifiable instance of the list.
     */
    QList<ElementType>& list() {
        onValueChanged();
        m_model.beginChange();
        return m_list;
    }

    void setList(const QList<ElementType>& newList) {
        list() = newList;
    }

    const QList<ElementType>& list() const {
        return m_list;
    }

    void addElement(ElementType element) {
        list().append(element);
    }

    /**
     * Returns the model object exposed to QML
     */
    ModelListModel& getModel() {
        return m_model;
    }

    operator const QList<ElementType>&() const {
        return m_list;
    }

    operator const QList<QVariant>() const {
        QList<QVariant> s;
        for(const auto& e : m_list)
            s.append(QVariant::fromValue(e));
        return s;
    }

    QList<ElementType> m_list;

    TheModelListModel m_model;
};

template<typename EnumType> class EnumListProperty: public SimpleTypeListProperty<EnumType> {

};


typedef SimpleTypeListProperty<int> intListProperty;
typedef SimpleTypeListProperty<QString> QStringListProperty;
typedef SimpleTypeListProperty<bool> boolListProperty;


/*
class Service1 : public BaseObject {

    Q_OBJECT

public:

    Service1() : m_property(*this, &Service1::propertyChanged) {
    }

    Q_SIGNAL void propertyChanged();

    int getA() {
        return m_property.getValue();
    }

    QProperty<int> m_property;

};

class Service2 : public BaseObject {

public:

    int getA() {
        qDebug() << "Service2 getA";
        return 8;
    }

};


class OtherService : public BaseObject {

    Q_OBJECT

public:

    int getInt() {
        return m_value;
    }

    Q_SIGNAL void propertyChanged();

    int m_value = 7;
};
*/

