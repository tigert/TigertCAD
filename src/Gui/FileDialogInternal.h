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
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include "FileDialog.h"


namespace Gui::FileDialogInternal
{

enum class NativeFileDialogMode : uint8_t
{
    OpenSingle,
    OpenMultiple,
    Save,
};

QString getFilterDisplayName(const FileDialog::Filter&, bool);

QStringList nativeFileDialog(
    NativeFileDialogMode mode,
    QWidget* parent,
    const QString& caption,
    const QString& startPath,
    const FileDialog::FilterList& filters,
    qsizetype& selectedFilterIndex,
    FileDialog::Options options
);

bool getPreferShowFilterPatterns();

GuiExport void normalizeSavePath(QString&, const FileDialog::Filter&);

}  // namespace Gui::FileDialogInternal
