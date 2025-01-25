module;

export module server: timeout_worker;

import :database;

namespace LambdaSnail::server
{

    export class timeout_worker
    {
        // Priority heap of time stamps
        // The data needs time stamp, database id, key, and key version
        // Trigger maintenance function periodically:
        //  - Peek the top of the heap, if the time has not yet passed, go back to sleep
        //  - Else, pop one by one until all expired entries are processed
        //      - Lock database
        //      - Find key
        //      - Must check key version so that it hasn't been updated


    };
}