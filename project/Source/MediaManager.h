/*
    MediaManager.h - This file is part of Element

    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
      * Adapted from Introjucer - http://juce.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ELEMENT_MEDIA_MANAGER_H
#define ELEMENT_MEDIA_MANAGER_H

#include "element/Juce.h"

namespace Element {

    class Session;

    class MediaType {
    public:

        enum ID {
            Audio,
            MidiFile,
            PatternFile,
            Instrument,
            Unsupported
        };

    private:
        ID type;
    };

    class MediaManager
    {
    public:

        MediaManager();
        virtual ~MediaManager();

        class Document
        {
        public:

            inline Document() { }
            virtual ~Document() { }
            virtual MediaPtr getMediaObject() = 0;
            virtual void fileHasBeenRenamed (const File& newFile) = 0;
            virtual File getFile() const = 0;
            virtual String getName() const = 0;
            virtual Session* getSession() const = 0;
            virtual String getState() const = 0;
            virtual String getType() const = 0;

            virtual bool hasFileBeenModifiedExternally() = 0;

            virtual bool isForFile (const File& file) const = 0;
            virtual bool isForNode (const ValueTree& node) const = 0;

            virtual bool loadedOk() const = 0;
            virtual bool needsSaving() const = 0;
            virtual bool save() = 0;
            virtual bool saveAs() = 0;
            virtual void reloadFromFile() = 0;
            virtual bool refersToSession (Session& session) const = 0;
            virtual void restoreState (const String& state) = 0;
        };

        bool canOpenFile (const File& file);
        Document* openFile (Session* session, const File& file);
        MediaPtr createObject (Session* session, const File& file);

        int getNumOpenDocuments() const;
        Document* getOpenDocument (int index) const;
        void clear();

        bool closeDocument (int index, bool saveIfNeeded);
        bool closeDocument (Document* document, bool saveIfNeeded);
        bool closeAll (bool askUserToSave);
        bool closeAllDocumentsUsingSession (Session& Session, bool saveIfNeeded);
        void closeFile (const File& f, bool saveIfNeeded);

        bool anyFilesNeedSaving() const;
        bool saveAll();
        FileBasedDocument::SaveResult saveIfNeededAndUserAgrees (Document* doc);

        void fileHasBeenRenamed (const File& oldFile, const File& newFile);
        void reloadModifiedFiles();

        class DocumentCloseListener
        {
        public:
            DocumentCloseListener() { }
            virtual ~DocumentCloseListener() { }
            virtual void documentAboutToClose (Document* document) = 0;
        };

        void addListener (DocumentCloseListener*);
        void removeListener (DocumentCloseListener*);

        class DocumentType
        {
        public:

            DocumentType() { }
            virtual ~DocumentType() { }
            virtual bool canOpenFile (const File& file) = 0;
            virtual Document* openFile (Session* session, const File& file) = 0;

        };

        void registerType (DocumentType* type);

    private:

        OwnedArray <Document> documents;
        OwnedArray <DocumentType> types;
        Array <DocumentCloseListener*> listeners;

    };

    class RecentDocumentList    : private MediaManager::DocumentCloseListener
    {
    public:

        RecentDocumentList();
        ~RecentDocumentList();

        void clear();

        void newDocumentOpened (MediaManager::Document* document);

        MediaManager::Document* getCurrentDocument() const       { return previousDocs.getLast(); }

        bool canGoToPrevious() const;
        bool canGoToNext() const;

        MediaManager::Document* getPrevious();
        MediaManager::Document* getNext();

        MediaManager::Document* getClosestPreviousDocOtherThan (MediaManager::Document* oneToAvoid) const;

        void restoreFromXml (Session& session, const XmlElement& xml);
        XmlElement* createXml() const;

    private:

        void documentAboutToClose (MediaManager::Document*);
        Array <MediaManager::Document*> previousDocs, nextDocs;

    };


}

#endif /* ELEMENT_MEDIA_MANAGER_H */
