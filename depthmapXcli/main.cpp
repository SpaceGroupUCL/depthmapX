// Copyright (C) 2017 Christian Sailer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include "commandlineparser.h"
#include "runmethods.h"
#include "performancewriter.h"
#include "modeparserregistry.h"

using namespace std;

int main(int argc, char *argv[])
{
    ModeParserRegistry registry;
    CommandLineParser args(registry);
    try{
        args.parse(argc, argv);
        if (!args.isValid())
        {
            if (args.printVersionMode())
            {
                args.printVersion();
            }
            else
            {
                args.printHelp();
            }
            return 0;
        }

        PerformanceWriter perfWriter(args.getTimingFile());

        args.run(perfWriter);
        perfWriter.write();

    }
    catch( std::exception &e)
    {
        cout << e.what() << "\n";
        args.printHelp();
        return -1;
    }
    return 0;
}
