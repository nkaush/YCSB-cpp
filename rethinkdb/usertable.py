#!/usr/bin/env python3
from rethinkdb import r
from typing import List

import argparse
import sys


def parse_args(args_list: List[str]):
    parser = argparse.ArgumentParser()

    parser.add_argument("-a", "--host", type=str, default="localhost")
    parser.add_argument("-p", "--port", type=int, default=28015)
    parser.add_argument("--db", type=str, default="test")
    parser.add_argument("-t", "--table", type=str, default="usertable")
    parser.add_argument("-d", "--durability", type=str, default="soft")

    return parser.parse_args(args_list)


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1].lower() not in ["create", "drop"]:
        print(f"Usage: {sys.argv[0]} create [args...]")
        print(f"       {sys.argv[0]} drop [args...]")
        exit(1)

    command = sys.argv[1].lower()
    args = parse_args(sys.argv[2:])
    conn = r.connect(host=args.host, port=args.port, db=args.db)
    
    if command == "create":
        result = r.db("test").table_create(
                args.table,
                replicas=3,
                durability=args.durability
            ).run(conn)
        assert result["tables_created"] == 1
    elif command == "drop":
        tables = r.table_list().run(conn)
        if args.table in tables:
            result = r.table_drop(args.table).run(conn)
            assert result["tables_dropped"] == 1
