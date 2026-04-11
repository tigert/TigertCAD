/***************************************************************************
 *   Copyright (c) 2026 The FreeCAD Project Association AISBL              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include <cstddef>
#include <memory>

// windows.h must be kept above commdlg.h and shlobj.h
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "FileDialogInternal.h"


using namespace Gui;

static std::unique_ptr<wchar_t[]> qStringToWCharArray(const QString& s, size_t reserveSize = 0)
{
    const size_t stringSize = s.size();
    wchar_t* result = new wchar_t[qMax(stringSize + 1, reserveSize)];
    s.toWCharArray(result);
    result[stringSize] = 0;
    return std::unique_ptr<wchar_t[]>(result);
}

/* Use the legacy Get{Open,Save}FileNameW functions as the Vista+ IFileDialog forces
 * pattern/extension display in filter lists, leading to exceedingly long entries as seen in
 * issue #23139.
 * Note neither this legacy function set nor IFileDialog are valid for UWP WinRT,
 * for which Windows::Storage::Pickers::FileOpenPicker will have to be used instead.
 */
QStringList FileDialogInternal::nativeFileDialog(
    NativeFileDialogMode mode,
    QWidget* parent,
    const QString& caption,
    const QString& startPath,
    const FileDialog::FilterList& filters,
    qsizetype& selectedFilterIndex,
    FileDialog::Options options
)
{
    const bool showPatterns = getPreferShowFilterPatterns();

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    if (parent) {
        ofn.hwndOwner = HWND(parent->winId());
    }

    QString flatFilter;
    for (const auto& filter : filters) {
        flatFilter += getFilterDisplayName(filter, showPatterns);
        flatFilter += QLatin1Char('\0');
        flatFilter += filter.patterns.join(QLatin1Char(';'));
        flatFilter += QLatin1Char('\0');
    }
    flatFilter += QLatin1Char('\0');
    auto ofnFilter = qStringToWCharArray(flatFilter);
    ofn.lpstrFilter = ofnFilter.get();

    if (selectedFilterIndex >= 0) {
        ofn.nFilterIndex = selectedFilterIndex + 1;  // OPENFILENAMEW index is 1-based
    }

    const QFileInfo startPathInfo(startPath);

    constexpr const DWORD SelectionBufferSize = 65535;
    auto selectedFile = std::make_unique<wchar_t[]>(SelectionBufferSize);
    startPathInfo.fileName().toWCharArray(selectedFile.get());
    selectedFile[startPathInfo.fileName().size()] = L'\0';

    ofn.nMaxFile = SelectionBufferSize;
    ofn.lpstrFile = selectedFile.get();

    auto initialDir = qStringToWCharArray(QDir::toNativeSeparators(startPath));
    ofn.lpstrInitialDir = initialDir.get();

    auto title = qStringToWCharArray(caption);
    ofn.lpstrTitle = title.get();

    ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST;
    if (mode == NativeFileDialogMode::OpenSingle || mode == NativeFileDialogMode::OpenMultiple) {
        ofn.Flags |= OFN_FILEMUSTEXIST;
    }

    BOOL ok = FALSE;
    if (mode == NativeFileDialogMode::OpenSingle) {
        ok = ::GetOpenFileNameW(&ofn);
    }
    else if (mode == NativeFileDialogMode::OpenMultiple) {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
        ok = ::GetOpenFileNameW(&ofn);
    }
    else /* (mode == NativeFileDialogMode::Save) */ {
        ok = ::GetSaveFileNameW(&ofn);
    }

    QStringList selected;
    // Always return a selected filter index >= 0, but
    // ofn.nFilterIndex, while 1-based, might be 0 if the user cancelled.
    selectedFilterIndex = ofn.nFilterIndex == 0 ? 0 : (ofn.nFilterIndex - 1);
    if (ok != FALSE) {
        const QString dir = QDir::cleanPath(QString::fromWCharArray(ofn.lpstrFile));
        selected += dir;
        if (ofn.Flags & OFN_ALLOWMULTISELECT) {
            const wchar_t* ptr = ofn.lpstrFile + dir.size() + 1;
            if (*ptr) {
                selected.clear();
                const QString path = dir + L'/';
                while (*ptr) {
                    const QString fileName = QString::fromWCharArray(ptr);
                    selected += path + fileName;
                    ptr += fileName.size() + 1;
                }
            }
        }
    }
    return selected;
}
