# @file
# build script for AutoBuild

import glob
import os
import subprocess
from askapdev.rbuild.builders import Scons as Builder
from askapdev.rbuild.utils import runcmd

# Set up the custom build step for generating the ODB persistence code from our
# data model

# Keep this one outside the odb_prebuild function so I can use it as a clean
# target for rbuild
odb_output_dir = os.path.abspath("./datamodel")

def odb_prebuild():

    # Target database. Using SQLite for now, but will
    # need to switch to a dynamic option later
    database = "sqlite"

    # Build some important paths
    odb_compiler = os.path.join(
        os.path.expandvars("$ASKAP_ROOT"),
        "3rdParty/odb/odb-2.4.0/odb/install/bin/odb")

    schema_dir = os.path.abspath("./schema")

    odb_includes = os.path.join(
        os.path.expandvars("$ASKAP_ROOT"),
        "3rdParty/odb/odb-2.4.0/libodb/install/include")

    # build the command
    cmd = [
        odb_compiler,  # The ODB compiler
        '--generate-query',  # Generate query support code
        '--generate-schema',  # Generate database schema SQL
        '--output-dir', odb_output_dir,  # output location for generated files
        '--database', database,
        '-I', odb_includes,  # ODB headers
        # '',  #
        ]

    # append the list of sources
    sources = glob.glob(os.path.join(schema_dir, "*.h"))
    cmd.extend(sources)

    if not os.path.exists(odb_output_dir):
        os.mkdir(odb_output_dir)

    # I want a failed ODB compile to abort the whole build. Using check_call
    # with the raised exception on failure seems the simplest way to achieve
    # this.
    # print(cmd)
    return subprocess.check_call(cmd)
    # return runcmd(cmd)[2]


b = Builder(".")
b.add_precallback(odb_prebuild)
b.add_extra_clean_targets(odb_output_dir)
b.build()
