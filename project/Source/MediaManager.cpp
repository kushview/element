/*
    MediaManager.cpp - This file is part of Element

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

#include "MediaManager.h"
#include "session/Session.h"

namespace Element {

    MediaManager::MediaManager() { }
    MediaManager::~MediaManager() { }

    bool
    MediaManager::anyFilesNeedSaving() const
    {
        for (int i = documents.size(); --i >= 0;)
        {
            Document* d = documents.getUnchecked (i);

            if (d->needsSaving())
                return true;
        }

        return false;
    }

    MediaPtr
    MediaManager::createObject (Session* session, const File &file)
    {
        SessionRef ref = session->makeRef();

        MediaPtr obj;
        if (Document* d = openFile (session, file))
            obj = d->getMediaObject();

        if (obj)
        {

        }

        ref.reset();
        return obj;
    }

    void MediaManager::clear()
    {
        documents.clear();
        //xx types.clear();
    }


    //==============================================================================
    bool MediaManager::canOpenFile (const File& file)
    {
        for (int i = types.size(); --i >= 0;)
            if (types.getUnchecked(i)->canOpenFile (file))
                return true;

        return false;
    }


    bool MediaManager::closeDocument (int index, bool saveIfNeeded)
    {
        if (Document* doc = documents [index])
        {
            if (saveIfNeeded)
                if (saveIfNeededAndUserAgrees (doc) != FileBasedDocument::savedOk)
                    return false;

            for (int i = listeners.size(); --i >= 0;)
                listeners.getUnchecked(i)->documentAboutToClose (doc);

            documents.remove (index);
            // IntrojucerApp::getCommandManager().commandStatusChanged();
        }

        return true;
    }

    bool MediaManager::closeAll (bool askUserToSave)
    {
        for (int i = getNumOpenDocuments(); --i >= 0;)
            if (! closeDocument (i, askUserToSave))
                return false;

        return true;
    }

    bool MediaManager::closeDocument (Document* document, bool saveIfNeeded)
    {
        return closeDocument (documents.indexOf (document), saveIfNeeded);
    }

    void
    MediaManager::closeFile (const File& f, bool saveIfNeeded)
    {
        for (int i = documents.size(); --i >= 0;)
        {
            Document* d = documents.getUnchecked (i);
            if (d->isForFile (f))
                closeDocument (i, saveIfNeeded);
        }
    }


    bool
    MediaManager::closeAllDocumentsUsingSession (Session& session, bool saveIfNeeded)
    {
        for (int i = documents.size(); --i >= 0;)
        {
            Document* d = documents.getUnchecked (i);

            if (d->refersToSession (session))
            {
                if (! closeDocument (i, saveIfNeeded))
                    return false;
            }
        }

        return true;
    }


    int MediaManager::getNumOpenDocuments() const
    {
        return documents.size();
    }

    MediaManager::Document*
    MediaManager::getOpenDocument (int index) const
    {
        return documents.getUnchecked (index);
    }


    MediaManager::Document*
    MediaManager::openFile (Session* session, const File& file)
    {
        for (int i = documents.size(); --i >= 0;)
            if (documents.getUnchecked(i)->isForFile (file))
                return documents.getUnchecked (i);

        Document* d = nullptr;

        for (int i = types.size(); --i >= 0 && d == nullptr;)
        {
            if (types.getUnchecked(i)->canOpenFile (file))
            {
                d = types.getUnchecked(i)->openFile (session, file);
                jassert (d != nullptr);
            }
        }

        jassert (d != nullptr);  // should always at least have been picked up by UnknownDocument

        if (d)
            documents.add (d);
        // IntrojucerApp::getCommandManager().commandStatusChanged();
        return d;
    }

    FileBasedDocument::SaveResult
    MediaManager::saveIfNeededAndUserAgrees (MediaManager::Document* doc)
    {
        if (! doc->needsSaving())
            return FileBasedDocument::savedOk;

        const int r = AlertWindow::showYesNoCancelBox (AlertWindow::QuestionIcon,
                                                       TRANS("Closing document..."),
                                                       TRANS("Do you want to save the changes to \"")
                                                           + doc->getName() + "\"?",
                                                       TRANS("Save"),
                                                       TRANS("Discard changes"),
                                                       TRANS("Cancel"));

        if (r == 1)  // save changes
            return doc->save() ? FileBasedDocument::savedOk
                               : FileBasedDocument::failedToWriteToFile;

        if (r == 2)  // discard changes
            return FileBasedDocument::savedOk;

        return FileBasedDocument::userCancelledSave;
    }

    bool MediaManager::saveAll()
    {
        for (int i = documents.size(); --i >= 0;)
        {
            Document* d = documents.getUnchecked (i);

            if (! d->save())
                return false;
        }

        return true;
    }

    void MediaManager::reloadModifiedFiles()
    {
        for (int i = documents.size(); --i >= 0;)
        {
            Document* d = documents.getUnchecked (i);

            if (d->hasFileBeenModifiedExternally())
                d->reloadFromFile();
        }
    }

    void MediaManager::fileHasBeenRenamed (const File& oldFile, const File& newFile)
    {
        for (int i = documents.size(); --i >= 0;)
        {
            Document* d = documents.getUnchecked (i);

            if (d->isForFile (oldFile))
                d->fileHasBeenRenamed (newFile);
        }
    }

    void
    MediaManager::registerType (DocumentType* type)
    {
        types.addIfNotAlreadyThere (type);
    }


    void
    MediaManager::addListener (DocumentCloseListener* dl)
    {
        listeners.add (dl);
    }

    void
    MediaManager::removeListener (DocumentCloseListener* dl)
    {
        listeners.removeAllInstancesOf (dl);
    }

    //==============================================================================
    RecentDocumentList::RecentDocumentList() { }
    RecentDocumentList::~RecentDocumentList() { }

    void
    RecentDocumentList::clear()
    {
        previousDocs.clear();
        nextDocs.clear();
    }

    void
    RecentDocumentList::newDocumentOpened (MediaManager::Document* document)
    {
        if (document != nullptr && document != getCurrentDocument())
        {
            nextDocs.clear();
            previousDocs.add (document);
        }
    }

    bool RecentDocumentList::canGoToPrevious() const
    {
        return previousDocs.size() > 1;
    }

    bool RecentDocumentList::canGoToNext() const
    {
        return nextDocs.size() > 0;
    }

    MediaManager::Document*
    RecentDocumentList::getPrevious()
    {
        if (! canGoToPrevious())
            return nullptr;

        nextDocs.insert (0, previousDocs.remove (previousDocs.size() - 1));
        return previousDocs.getLast();
    }

    MediaManager::Document*
    RecentDocumentList::getNext()
    {
        if (! canGoToNext())
            return nullptr;

        MediaManager::Document* d = nextDocs.remove (0);
        previousDocs.add (d);
        return d;
    }

    MediaManager::Document*
    RecentDocumentList::getClosestPreviousDocOtherThan (MediaManager::Document* oneToAvoid) const
    {
        for (int i = previousDocs.size(); --i >= 0;)
            if (previousDocs.getUnchecked(i) != oneToAvoid)
                return previousDocs.getUnchecked(i);

        return nullptr;
    }

    void
    RecentDocumentList::documentAboutToClose (MediaManager::Document* document)
    {
        previousDocs.removeAllInstancesOf (document);
        nextDocs.removeAllInstancesOf (document);

        jassert (! previousDocs.contains (document));
        jassert (! nextDocs.contains (document));
    }

    static void restoreDocList (Session& session, Array <MediaManager::Document*>& list, const XmlElement* xml)
    {
    #if 0
        if (xml != nullptr)
        {
            OpenDocumentManager& odm = IntrojucerApp::getApp().openDocumentManager;

            forEachXmlChildElementWithTagName (*xml, e, "DOC")
            {
                const File file (e->getStringAttribute ("file"));

                if (file.exists())
                {
                    if (MediaManager::Document* doc = odm.openFile (&session, file))
                    {
                        doc->restoreState (e->getStringAttribute ("state"));

                        list.add (doc);
                    }
                }
            }
        }
    #endif
    }

    void RecentDocumentList::restoreFromXml (Session& session, const XmlElement& xml)
    {
        clear();
        if (xml.hasTagName ("recent-documents"))
        {
            restoreDocList (session, previousDocs, xml.getChildByName ("previous"));
            restoreDocList (session, nextDocs,     xml.getChildByName ("next"));
        }
    }

    static void saveDocList (const Array <MediaManager::Document*>& list, XmlElement& xml)
    {
        for (int i = 0; i < list.size(); ++i)
        {
            const MediaManager::Document& doc = *list.getUnchecked(i);

            XmlElement* e = xml.createNewChildElement ("DOC");

            e->setAttribute ("file", doc.getFile().getFullPathName());
            e->setAttribute ("state", doc.getState());
        }
    }

    XmlElement* RecentDocumentList::createXml() const
    {
        XmlElement* xml = new XmlElement ("RECENT_DOCUMENTS");

        saveDocList (previousDocs, *xml->createNewChildElement ("PREVIOUS"));
        saveDocList (nextDocs,     *xml->createNewChildElement ("NEXT"));

        return xml;
    }

}
