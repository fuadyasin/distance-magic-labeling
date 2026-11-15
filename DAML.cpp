#include "cli.h"

int main(int argc, char* argv[])
{
	return runCli(LabelingKind::DistanceAntimagic, argc, argv);
}
