# SPDX-License-Identifier: LGPL-2.1-or-later
"""**************************************************************************
*                                                                          *
*   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
*                                                                          *
*   This file is part of FreeCAD.                                          *
*                                                                          *
*   FreeCAD is free software: you can redistribute it and/or modify it     *
*   under the terms of the GNU Lesser General Public License as            *
*   published by the Free Software Foundation, either version 2.1 of the   *
*   License, or (at your option) any later version.                        *
*                                                                          *
*   FreeCAD is distributed in the hope that it will be useful, but         *
*   WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
*   Lesser General Public License for more details.                        *
*                                                                          *
*   You should have received a copy of the GNU Lesser General Public       *
*   License along with FreeCAD. If not, see                                *
*   <https://www.gnu.org/licenses/>.                                       *
*                                                                          *
***************************************************************************/"""

import threading
import time
import unittest

import FreeCAD
import FreeCADGui

# ---------------------------------------------------------------------------
# define the functions to test the FreeCAD Gui Document code
# ---------------------------------------------------------------------------


class TestGuiDocument(unittest.TestCase):
    def setUp(self):
        # Create a new document
        self.doc = FreeCAD.newDocument("TestDoc")

    def tearDown(self):
        # Close the document
        FreeCAD.closeDocument("TestDoc")

    def testGetTreeRootObject(self):
        # Create objects at the root level
        group1 = self.doc.addObject("App::DocumentObjectGroup", "Group1")
        group2 = self.doc.addObject("App::DocumentObjectGroup", "Group2")
        obj1 = self.doc.addObject("App::FeaturePython", "RootObject1")
        part1 = self.doc.addObject("App::Part", "Part1")

        # Create App::Parts and groups with objects in them
        part1_obj = part1.newObject("App::FeaturePython", "Part1_Object")
        group3 = group2.newObject("App::DocumentObjectGroup", "Group1")
        group1_obj = group3.newObject("App::FeaturePython", "Group1_Object")

        # Fetch the root objects using getTreeRootObjects
        root_objects = FreeCADGui.getDocument("TestDoc").TreeRootObjects

        # Check if the new function returns the correct root objects
        expected_root_objects = [group1, group2, obj1, part1]
        self.assertEqual(set(root_objects), set(expected_root_objects))

    def testViewObjectRequiresMainThread(self):
        obj = self.doc.addObject("App::FeaturePython", "ThreadGuard")

        result = {}

        def access_view_object():
            try:
                _ = obj.ViewObject
            except Exception as exc:  # pragma: no cover - exercised through assertion below
                result["exception"] = exc

        worker = threading.Thread(target=access_view_object)
        worker.start()
        worker.join()

        self.assertIn("exception", result)
        self.assertIsInstance(result["exception"], RuntimeError)
        self.assertIn("main thread", str(result["exception"]).lower())

    def testRefreshFallsBackToSyncForFeaturePython(self):
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document")
        old_async = params.GetBool("EnableAsyncRecompute", False)

        class RefreshProxy:
            def __init__(self):
                self.executed_thread_id = None

            def execute(self, obj):
                self.executed_thread_id = threading.get_ident()
                time.sleep(0.05)

        proxy = RefreshProxy()
        obj = self.doc.addObject("App::FeaturePython", "PythonFeature")
        obj.Proxy = proxy
        obj.touch()

        try:
            params.SetBool("EnableAsyncRecompute", True)

            start = time.monotonic()
            FreeCADGui.runCommand("Std_Refresh", 0)
            elapsed = time.monotonic() - start
        finally:
            params.SetBool("EnableAsyncRecompute", old_async)

        self.assertEqual(proxy.executed_thread_id, threading.get_ident())
        self.assertGreaterEqual(elapsed, 0.04)
