//////////////////////////////////////////////////////////////////////////
// File: MessageHandler.h                                               //
// Date: 2010-04-21                                                     //
// Desc: Handles messages from other windows, which are detected        //
//   by the hook DLL.                                                   //
//////////////////////////////////////////////////////////////////////////

/********************************************************************
  Window Detective
  Copyright (C) 2010-2012 XTAL256

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "inspector.h"
#include "hook/Hook.h"


#define HANDLER_WINDOW_CLASS_NAME  L"MessageHandlerWindow"


/* UI widgets can inherit this to be notified by the MessageHandler */
class WindowMessageListener {
public:
    virtual void messageAdded(WindowMessage* msg) = 0;
    virtual void messageRemoved(WindowMessage* msg) = 0;
};


class MessageHandler {
private:
    static HWND hwndReceiver;         // Window to receive messages from DLL
    static bool isWindowClassCreated;
    static void createWindowClass();
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    QMap<Window*,WindowMessageListener*> listeners;
    QMap<Window*,QList<WindowMessage*>> windowMessages;
public:
    static MessageHandler& current();  // Singleton instance

    MessageHandler();
    ~MessageHandler();

    bool installHook();
    bool removeHook();
    bool addMessageListener(WindowMessageListener* l, Window* wnd);
    void removeMessageListener(WindowMessageListener* l);
    void removeAllListeners();
    void removeMessages(Window* wnd);
    void processMessage(const MessageEvent& msg);
    void writeMessagesToXml(Window* window, QXmlStreamWriter& stream);
};

/* C++ Wrapper for Hook DLL */
// TODO: Perhaps put this in a file called DllInterface.

class HookDll {
public:
    static void initialize(HWND hwnd, DWORD pid) { Initialize(hwnd, pid); }

    static DWORD install() { return InstallHook(); }

    static DWORD remove() { return RemoveHook(); }

    static QList<HWND> getWindowsToMonitor() {
        HWND* handles = new HWND[MAX_WINDOWS];
        int size = MAX_WINDOWS;
        QList<HWND> list;

        GetWindowsToMonitor(handles, &size);
        for (int i = 0; i < size; ++i) {
            list.append(handles[i]);
        }

        delete[] handles;
        return list;
    }

    static bool addWindowToMonitor(HWND handle) {
        return AddWindowToMonitor(handle);
    }

    static bool removeWindowToMonitor(HWND handle) {
        return RemoveWindowToMonitor(handle);
    }

    static bool removeAllWindowsToMonitor() {
        return RemoveAllWindowsToMonitor();
    }

    static bool startGetInfo(HWND handle) {
        return StartGetInfo(handle);
    }

    static bool stopGetInfo(HWND handle) {
        return StopGetInfo(handle);
    }
};


#endif   // MESSAGE_HANDLER_H
