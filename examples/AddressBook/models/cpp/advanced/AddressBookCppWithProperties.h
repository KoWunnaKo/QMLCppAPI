/*
 *   This file is part of the QMLCppAPI project
 *   Copyright (C) 2017 Pelagicore AB
 *   SPDX-License-Identifier: LGPL-2.1
 *   This file is subject to the terms of the <license name> licence.
 *   Please see the LICENSE file for details. 
 */

#pragma once

#include "addressbook/AddressBookPropertyAdapter.h"

using namespace addressbook;

/**
 * C++ Implementation of the AddressBook API, based on the Property template class
 */
class AddressBookCppWithProperties: public AddressBookPropertyAdapter {

    Q_OBJECT

public:
    AddressBookCppWithProperties(QObject* parent = nullptr) :
            AddressBookPropertyAdapter(parent) {
        m_isLoaded = true;
        setImplementationID("C++ model implemented with properties");
    }

    void selectContact(int contactId) override {
        auto* contact = m_contacts.elementPointerById(contactId);
        if (contact != nullptr)
            m_currentContact = *contact;
        else
            qWarning() << "Unknown elementID " << contactId;
    }

    void updateContact(int contactId, Contact newContact) override {
        auto* contact = m_contacts.elementPointerById(contactId);

        Q_ASSERT(contact != nullptr);
        if (contact != nullptr) {
            *contact = newContact;
            contact->setId(contactId);
            qDebug() << "Updated contact " << newContact.toString();
            currentContactChanged();
            contactsChanged();
        }
        else
            qWarning() << "Unknown elementID " << newContact.id();
    }

    void createNewContact() override {
        if (m_contacts.list().size() < 3) {
            qDebug() << "C++ createNewContact called";

            static int nextContactIndex = 0;

            Contact newContact;
            newContact.setname("New contact " + QString::number(nextContactIndex));
            newContact.setnumber("089 " + QString::number(nextContactIndex));
            m_contacts.addElement(newContact);
            nextContactIndex++;

            // Set as current contact
            m_currentContact = newContact;

            contactCreated(newContact);
        }
        else {
            contactCreationFailed(FailureReason::Full);
        }

    }

};

