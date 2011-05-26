/////////////////////////////////////////////////////////////////////
// File: MessageHandler.h                                          //
// Date: 21/4/10                                                   //
// Desc: Handles messages from other windows, which are detected   //
//   by the hook DLL.                                              //
/////////////////////////////////////////////////////////////////////

/********************************************************************
  Window Detective
  Copyright (C) 2010-2011 XTAL256

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

#include "window_detective/include.h"
#include "MessageHandler.h"
#include "WindowManager.h"
#include "window_detective/Settings.h"
#include "window_detective/Logger.h"
#include <QStringBuilder>
using namespace inspector;

MessageHandler* MessageHandler::Current = NULL;
bool MessageHandler::isWindowClassCreated = false;
HWND MessageHandler::hwndReceiver = NULL;

/**********************/
/*** Static methods ***/
/**********************/

/*------------------------------------------------------------------+
| Creates the window class that is used for receiving messages      |
| from the DLL injected into remote processes.                      |
+------------------------------------------------------------------*/
void MessageHandler::createWindowClass() {
    if (MessageHandler::isWindowClassCreated)
        return;   // Can only be called once

    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)MessageHandler::wndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hIcon = NULL;
    wndclass.hInstance = GetModuleHandle(NULL);
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = HANDLER_WINDOW_CLASS_NAME;

    if (RegisterClass(&wndclass)) {
        MessageHandler::isWindowClassCreated = true;
    }
    else {
        Logger::osError(TR("Could not register message handler window. "
                    "Window Detective will not be able to monitor "
                    "messages of other windows."));
    }
}

/*------------------------------------------------------------------+
| Window callback procedure for message handler.                    |
| Receives WM_COPYDATA messages from the DLL that is injected into  |
| remote processes. The data sent is the window message that the    |
| remote window has received.                                       |
+------------------------------------------------------------------*/
LRESULT CALLBACK MessageHandler::wndProc(HWND hwnd, UINT msg,
                    WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COPYDATA) {
        COPYDATASTRUCT* dataStruct = (COPYDATASTRUCT*)lParam;
        MessageEvent* evnt = (MessageEvent*)dataStruct->lpData;
        uint type = evnt->type;

        // Check if it's a message that we are monitoring
        if ((type & MessageTypeMask) != 0)
            MessageHandler::current()->messageEvent(*evnt);

        // Check if it's an message that requires us to update
        if (((type & UpdateFlag) == UpdateFlag) &&
            ((type & MessageTypeMask) != MessageReturn))
            MessageHandler::current()->updateEvent(*evnt);

        return TRUE;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*------------------------------------------------------------------+
| Initialize singleton instance.                                    |
+------------------------------------------------------------------*/
void MessageHandler::initialize() {
    if (Current != NULL) delete Current;
    Current = new MessageHandler();
}


/************************/
/*** Instance methods ***/
/************************/

/*------------------------------------------------------------------+
| Constructor                                                       |
+------------------------------------------------------------------*/
MessageHandler::MessageHandler() :
    windowMessages(),
    listeners() {

    if (!MessageHandler::isWindowClassCreated)
        createWindowClass();

    hwndReceiver = CreateWindowEx(0, HANDLER_WINDOW_CLASS_NAME,
                L"", 0, 0, 0, 0, 0, NULL, NULL,
                GetModuleHandle(NULL), NULL);
    if (!hwndReceiver) {
        Logger::osError(TR("Could not create message handler window. "
                    "Window Detective will not be able to monitor "
                    "messages of other windows."));
    }
    HookDll::initialize(hwndReceiver, GetCurrentProcessId());
    installHook();
}

/*------------------------------------------------------------------+
| Destructor                                                        |
+------------------------------------------------------------------*/
MessageHandler::~MessageHandler() {
    removeHook();
    if (!DestroyWindow(hwndReceiver)) {
        Logger::osWarning(TR("Could not destroy message handler window"));
    }
}

/*------------------------------------------------------------------+
| Adds a listener object to the list of listeners. That object      |
| will then get notified whenever there is a new message from the   |
| given window. If the window is NULL, it will be notified of       |
| messages from all windows.                                        |
+------------------------------------------------------------------*/
void MessageHandler::addMessageListener(WindowMessageListener* l,
                                        Window* window) {
    if (!listeners.contains(window)) {
        listeners.insert(window, l);
        if (!HookDll::addWindowToMonitor(window->getHandle())) {
            Logger::osError(TR("Could not monitor messages for window %1."
                        "Window Detective can monitor a maximum of %2 windows.")
                        .arg(String::number(MAX_WINDOWS), window->getDisplayName()));
        }
    }
}

/*------------------------------------------------------------------+
| Removes the listener object from the list of listeners. If it     |
| is listening to more than one window, all references will be      |
| removed.                                                          |
+------------------------------------------------------------------*/
void MessageHandler::removeMessageListener(WindowMessageListener* l) {
    QMap<Window*,WindowMessageListener*>::const_iterator i;
    WindowList keys;

    for (i = listeners.constBegin(); i != listeners.constEnd(); i++) {
         if (i.value() == l)
            keys.append(i.key());
    }
    for (int i = 0; i < keys.size(); i++) {
        listeners.remove(keys[i]);
        HookDll::removeWindowToMonitor(keys[i]->getHandle());
    }
}

/*------------------------------------------------------------------+
| Removes all listeners and stops monitoring their windows.         |
+------------------------------------------------------------------*/
void MessageHandler::removeAllListeners() {
    HookDll::removeAllWindowsToMonitor();
    listeners.clear();
}

/*------------------------------------------------------------------+
| Writes the list of messages for the window to an XML file stream. |
+------------------------------------------------------------------*/
void MessageHandler::writeMessagesToXml(Window* window, QXmlStreamWriter& stream) {
    if (!windowMessages.contains(window)) return;

    stream.writeComment("\n" %
            TR("Messages for window ") % window->getDisplayName() % "\n" %
            TR("Created by Window Detective") % "\n");

    stream.writeStartElement("messageList");
    QList<WindowMessage*>& messages = windowMessages[window];
    QList<WindowMessage*>::const_iterator i;
    for (i = messages.constBegin(); i != messages.constEnd(); i++) {
        (*i)->toXmlStream(stream);
    }
    stream.writeEndElement();
}

/*------------------------------------------------------------------+
| Installs a global (system-wide) hook to monitor messages being    |
| sent to and received by windows. The DLL is injected into each    |
| process that has a message queue.                                 |
+------------------------------------------------------------------*/
bool MessageHandler::installHook() {
    // Call DLL to set hook
    DWORD result = HookDll::install();
    if (!result) {
        Logger::osError(result, TR("Failed to install message hook"));
        return false;
    }

    // The DLL won't be mapped into a remote process until a message is
    // actually sent to (some window of) the hooked thread.
    // So force DLL to inject immediately by sending each window a message
    SendMessageTimeout(HWND_BROADCAST, WM_NULL,
                       0, 0, SMTO_ABORTIFHUNG, 10, &result);
    return true;
}

bool MessageHandler::removeHook() {
    // Call DLL to remove hook
    DWORD result = HookDll::remove();
    if (result != S_OK) {
        Logger::osError(result, TR("Failed to remove message hook"));
        return false;
    }

    // As with installing the hook, the DLL won't be unmapped until a
    // window receives a message. If the DLL is not unmapped now, it could
    // stay in the remote process for a while (if it's not processing
    // messages). So we wait a bit longer here to give it more time.
    SendMessageTimeout(HWND_BROADCAST, WM_NULL,
                       0, 0, SMTO_NORMAL, 100, &result);
    return true;
}

/*------------------------------------------------------------------+
| Event handler for messages which update a window.                 |
+------------------------------------------------------------------*/
void MessageHandler::updateEvent(const MessageEvent& e) {
    WindowManager* manager = WindowManager::current();
    if (e.messageId == WM_CREATE) {
        manager->addWindow(e.hwnd);
    }
    else if (e.messageId == WM_DESTROY) {
        manager->removeWindow(e.hwnd);
    }
    else {  // changed
        // TODO: Can't do this yet, as these updates will inevitably send a message
        // to the window, which in turn will call this again. This can result in
        // undefined behaviour if some variable is not yet initialized.
        // To solve this, i need to pass the MSG params (and data) here and use
        // that to update the window. e.g. on WM_MOVE, get hi & lo word of lParam.
        // This will have to be done when i do the SelfDefinedStructure thing.

        Window* window = manager->find(e.hwnd);
        if (!window) return;

        /*switch (e.messageId) {    // Only update what's necessary
          case WM_SETTEXT: {
              window->updateText();
              break;
          }
          case WM_MOVE:
          case WM_SIZE:
          case WM_STYLECHANGED: {
              window->updateCommonInfo();
              break;
          }
          case WM_SHOWWINDOW: {
              window->updateFlags();
              break;
          }
          case WM_SETICON: {
              window->updateIcon();
              break;
          }
          default:
              window->update();   // Just update it all otherwise
        }*/
        window->fireUpdateEvent(WindowChanged);

        // Update children if necessary.
        if (e.messageId == WM_MOVE || e.messageId == WM_SIZE) {
            foreach (Window* child, window->getDescendants()) {
                //child->updateCommonInfo();
                child->fireUpdateEvent(MinorChange);
            }
        }
    }
}

/*------------------------------------------------------------------+
| Event handler for messages from a monitored window.               |
+------------------------------------------------------------------*/
void MessageHandler::messageEvent(const MessageEvent& e) {
    Window* window = WindowManager::current()->find(e.hwnd);
    if (!window) {
        Logger::warning(TR("Message %1 from unknown window %2")
                    .arg(WindowMessage::nameForId(e.messageId),
                         hexString((uint)e.hwnd)));
        return;
    }
    if (!windowMessages.contains(window) && !listeners.contains(window)) {
        Logger::warning(TR("Not monitoring window: %1").arg(window->getDisplayName()));
        return;
    }

    QList<WindowMessage*>& messages = windowMessages[window];
    WindowMessage* message = NULL;

    if ((e.type & MessageTypeMask) == MessageReturn) {
        // Attempt to set the return value for the last recieved message.
        // Most messages should return before another is processed, but we will check
        // if the last message is the same ID just in case it is a different one
        if (messages.isEmpty()) {
            Logger::debug(TR("Message list is empty when recieving MessageReturn event.\n"
                            "message ID = %1, window = %2")
                            .arg(String::number(e.messageId), window->getDisplayName()));
            return;
        }
        message = messages.last();
        if (message->id == e.messageId) {
            message->returnValue = e.returnValue;
            listeners.value(window)->messageReturned(message);
        }
    }
    else {
        // Create a new message and add it to the list
        message = new WindowMessage(window, e.messageId,
                                    e.wParam, e.lParam, e.returnValue);
        
        // Remove any messages that exceed the max limit
        /*TODO: while (messages.size() > maxMessages) {
            WindowMessage* oldestMessage = messages.takeFirst();
            listeners.value(window)->messageRemoved(oldestMessage);
            delete oldestMessage;
        }*/

        // Add the new message
        messages.append(message);
        listeners.value(window)->messageAdded(message);
    }
}
