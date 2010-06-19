/////////////////////////////////////////////////////////////////////
// File: StringRenderer.h                                          //
// Date: 25/3/10                                                   //
// Desc: Provides functions for converting various data types and  //
//   objects to a string representation for display.               //
/////////////////////////////////////////////////////////////////////

#ifndef STRING_RENDERER_H
#define STRING_RENDERER_H

#include "window_detective/include.h"
#include "inspector/inspector.h"
using namespace inspector;

/* TODO: something like this...

class AbstractStringFormatter {
public:
    AbstractStringFormatter();
    ~AbstractStringFormatter();

    virtual String labelForDisplay() = 0;
}

class RectangleStringFormatter : public AbstractStringFormatter {
public:
    RectangleStringFormatter(QRect);

    String labelForDisplay() {
        return "(" + ...
    }
}

*/

/*** These functions return a plain string representation ***/

// Primitive and built-in types
// TODO: Is there any way to use a single function for all primitives?
String stringLabel(int value);
inline String stringLabel(uint value) { return stringLabel((int)value); }
String stringLabel(bool value);
inline String stringLabel(String value) { return value; }

// Objects and typedefs
String stringLabel(HWND value);
String stringLabel(const QRect& value);
String stringLabel(const QPoint& value);
String stringLabel(const QSize& value);
String stringLabel(WindowClass* value);
String stringLabel(const WindowStyleList& value);


/*** These functions return a string in HTML format ***/

inline String htmlLabel(int value) { return stringLabel(value); }
inline String htmlLabel(uint value) { return stringLabel(value); }
inline String htmlLabel(bool value) { return stringLabel(value); }
String htmlLabel(String value);

inline String htmlLabel(HWND value) { return stringLabel(value); }
inline String htmlLabel(const QRect& value) { return stringLabel(value); }
inline String htmlLabel(const QPoint& value) { return stringLabel(value); }
inline String htmlLabel(const QSize& value) { return stringLabel(value); }
inline String htmlLabel(WindowClass* value) { return stringLabel(value); }
String htmlLabel(const WindowStyleList& value);

#endif   // STRING_RENDERER_H