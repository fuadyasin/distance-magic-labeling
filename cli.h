#pragma once

#include "labeling.h"

// Shared command-line driver behind the DML, CDML and DAML executables.
int runCli(LabelingKind kind, int argc, char* argv[]);
