from rethinkdb import RethinkDB
import sys

r = RethinkDB()

# TODO need to properly integrate this into the run_workload script

res = r.connect( "localhost", 28015).repl()
stats = r.db('rethinkdb').table('_debug_stats')

print(stats.run())