#!/usr/bin/env python
"""This module is used to find all of the layout test files used by Chromium
(across all platforms). It exposes one public function - GatherTestFiles() -
which takes an optional list of paths. If a list is passed in, the returned
list of test files is constrained to those found under the paths passed in,
i.e. calling GatherTestFiles(["LayoutTests/fast"]) will only return files
under that directory."""

import glob
import os

# When collecting test cases, we include any file with these extensions.
_supported_file_extensions = set(['.html', '.shtml', '.xml', '.xhtml', '.pl',
                                  '.php', '.svg'])
# When collecting test cases, skip these directories
_skipped_directories = set(['.svn', '_svn', 'resources', 'script-tests'])


def gather_test_files(port, paths):
    """Generate a set of test files and return them.

    Args:
      paths: a list of command line paths relative to the webkit/tests
          directory. glob patterns are ok.
    """
    paths_to_walk = set()
    # if paths is empty, provide a pre-defined list.
    if paths:
        for path in paths:
            # If there's an * in the name, assume it's a glob pattern.
            path = os.path.join(port.layout_tests_dir(), path)
            if path.find('*') > -1:
                filenames = glob.glob(path)
                paths_to_walk.update(filenames)
            else:
                paths_to_walk.add(path)
    else:
        paths_to_walk.add(port.layout_tests_dir())

    # Now walk all the paths passed in on the command line and get filenames
    test_files = set()
    for path in paths_to_walk:
        if os.path.isfile(path) and _has_supported_extension(path):
            test_files.add(os.path.normpath(path))
            continue

        for root, dirs, files in os.walk(path):
            # don't walk skipped directories and sub directories
            if os.path.basename(root) in _skipped_directories:
                del dirs[:]
                continue

            for filename in files:
                if _has_supported_extension(filename):
                    filename = os.path.join(root, filename)
                    filename = os.path.normpath(filename)
                    test_files.add(filename)

    return test_files


def _has_supported_extension(filename):
    """Return true if filename is one of the file extensions we want to run a
    test on."""
    extension = os.path.splitext(filename)[1]
    return extension in _supported_file_extensions
