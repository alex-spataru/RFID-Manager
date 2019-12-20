/*
 * Copyright (c) 2019 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <QApplication>
#include <QStyleFactory>

#include "AppInfo.h"
#include "MainWindow.h"

/**
 * @brief Main entry point of the application
 * @param argc argument count
 * @param argv argument data
 * @return @c qApp event loop exit code
 */
int main(int argc, char** argv)
{
   // Set application attributes
   QApplication::setApplicationName(APP_NAME);
   QApplication::setApplicationVersion(APP_VERSION);
   QApplication::setAttribute(Qt::AA_NativeWindows, true);
   QApplication::setAttribute(Qt::AA_DisableHighDpiScaling, true);

   // Create QApplication and change widget style to Fusion
   QApplication app(argc, argv);
   app.setStyle(QStyleFactory::create("Fusion"));

   // Increase font size for better reading
   QFont f = QApplication::font();
   f.setPointSize(f.pointSize() + 2);
   app.setFont(f);

   // Generate dark color palette
   QPalette p;
   p.setColor(QPalette::Text, QColor(0xff, 0xff, 0xff));
   p.setColor(QPalette::Base, QColor(0x2a, 0x2a, 0x2a));
   p.setColor(QPalette::Dark, QColor(0x23, 0x23, 0x23));
   p.setColor(QPalette::Link, QColor(0x2a, 0x82, 0xda));
   p.setColor(QPalette::Window, QColor(0x35, 0x35, 0x35));
   p.setColor(QPalette::Shadow, QColor(0x14, 0x14, 0x14));
   p.setColor(QPalette::Button, QColor(0x35, 0x35, 0x35));
   p.setColor(QPalette::Highlight, QColor(0x2a, 0x82, 0xda));
   p.setColor(QPalette::BrightText, QColor(0xff, 0x00, 0x00));
   p.setColor(QPalette::WindowText, QColor(0xff, 0xff, 0xff));
   p.setColor(QPalette::ButtonText, QColor(0xff, 0xff, 0xff));
   p.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xff));
   p.setColor(QPalette::ToolTipText, QColor(0xff, 0xff, 0xff));
   p.setColor(QPalette::AlternateBase, QColor(0x42, 0x42, 0x42));
   p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
   p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x7f, 0x7f, 0x7f));
   p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0x50, 0x50, 0x50));
   p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x7f, 0x7f, 0x7f));
   p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x7f, 0x7f, 0x7f));
   p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(0x7f, 0x7f,0x7f));

   // Change application palette
   app.setPalette(p);

   // Create MainWindow instance
   MainWindow window;
   window.showNormal();

   // Begin qApp event loop
   return app.exec();
}
