#!/usr/bin/env python2

import argparse
import datetime
import inspect
import logging
import os
import re
import subprocess
import sys
import time

ansi_brown = "\x1b[0;33;40m"
ansi_yello_bold = "\x1b[1;33;40m"
ansi_lblue = "\x1b[0;36;40m"
ansi_pinky = "\x1b[0;35;40m"
ansi_reset = "\x1b[0m"

log_ts = None
def log_time():
  global log_ts
  if log_ts == None:
    log_ts = time.time()
  ts = time.time()
  delta_ms = int((ts - log_ts) * 1000)
  log_ts = ts
  return "{0: >6}".format(delta_ms)

def log_prefix():
  line = inspect.stack()[2][2]
  function = inspect.stack()[2][3]
  return ansi_lblue + "{0} {1}:{2}".format(log_time(), function, line) + ansi_reset

def debug(*args):
  parts = [log_prefix()]
  for s in args:
    parts.append(str(s))
  logging.debug(" ".join(parts))

def info(*args):
  parts = [log_prefix()]
  for s in args:
    parts.append(str(s))
  logging.info(" ".join(parts))

def cmd(s):
  logging.debug(log_prefix() + " Executing: " + ansi_brown + s + ansi_reset)
  return s

def run(s):
  proc = subprocess.Popen(cmd(s), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
  ret = proc.wait()
  out, err = proc.communicate()
  for line in out.split("\n"):
    debug(ansi_pinky + line + ansi_reset)
  debug(ansi_pinky + "Exit code: " + str(ret))
  if ret != 0:
    print(err)
    raise Exception("Command failed!")
  return out

def natsorted(ls):
  dre = re.compile(r'(\d+)')
  return sorted(ls, key=lambda l: [int(s) if s.isdigit() else s.lower() for s in re.split(dre, l)])

def get_last_version():
  tags = natsorted(run("git tag -l 'v*'").split("\n"))
  return tags[-1]

def inc_version(v, feature=False, breakage=False):
  parts = v.split(".")

  while len(parts) < 3:
    parts.append("0")
  assert len(parts) == 3

  if breakage:
    del parts[-1]
    parts[-2] = "v" + str(int(parts[-2].replace("v", "")) + 1)
    parts[-1] = "0"
  elif feature:
    del parts[-1]
    parts[-1] = str(int(parts[-1]) + 1)
  else:
    parts[-1] = str(int(parts[-1]) + 1)

  return ".".join([s for s in parts if s])

def assert_equal(a, b):
  if a != b:
    info(a, "!=", b)
    assert(False)

def test_inc_version():
  # auto (fix change)
  assert_equal(inc_version("v0.14.6"), "v0.14.7")
  assert_equal(inc_version("v15"    ), "v15.0.1")
  assert_equal(inc_version("v15.0"  ), "v15.0.1")
  assert_equal(inc_version("v15.0.0"), "v15.0.1")
  assert_equal(inc_version("v16.1"  ), "v16.1.1")
  assert_equal(inc_version("v16.1.3"), "v16.1.4")
  assert_equal(inc_version("v16.10" ), "v16.10.1")
  # feature
  assert_equal(inc_version("v15",    True), "v15.1")
  assert_equal(inc_version("v15.0",  True), "v15.1")
  assert_equal(inc_version("v16.1",  True), "v16.2")
  assert_equal(inc_version("v16.1.7",True), "v16.2")
  assert_equal(inc_version("v16.10", True), "v16.11")
  # breakage
  assert_equal(inc_version("v15",    False, True), "v16.0")
  assert_equal(inc_version("v15.0",  True,  True), "v16.0")
  assert_equal(inc_version("v15.0.1",False, True), "v16.0")
  assert_equal(inc_version("v15.2",  True,  True), "v16.0")
  assert_equal(inc_version("v15.10", False, True), "v16.0")

def replace_in_file(path, before, after):
  with open(path, "r+") as f:
    old = f.read()
    new = old.replace(before, after)
    f.seek(0)
    f.write(new)
    f.truncate(len(new))

def update_man(path, version, date):
  with open(path, "r+") as f:
    lines = f.read().split("\n")
    # # TINT2 1 "2017-03-26" 0.14.1
    parts = lines[0].split()
    parts[-2] = '"' + date + '"'
    parts[-1] = version
    lines[0] = " ".join(parts)
    f.seek(0)
    new = "\n".join(lines)
    f.write(new)
    f.truncate(len(new))
  run("cd doc ; ./generate-doc.sh")

def update_log(path, version, date):
  with open(path, "r+") as f:
    lines = f.read().split("\n")
    f.seek(0)
    assert lines[0].endswith("master")
    lines[0] = date + " " + version
    new = "\n".join(lines)
    f.write(new)
    f.truncate(len(new));

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--fix", action="store_true")
  parser.add_argument("--feature", action="store_true")
  parser.add_argument("--breakage", action="store_true")
  args = parser.parse_args()

  if args.fix == False and args.feature == False and args.breakage == False:
    print( "Type of version change is required, but not specified." )
    print( "Wild version bumps are discouraged." )
    print( "Options: --fix, --feature, --breakage" )
    sys.exit(1)

  logging.basicConfig(format=ansi_lblue + "%(asctime)s %(pathname)s %(levelname)s" + ansi_reset + " %(message)s", level=logging.DEBUG)
  test_inc_version()

  # Read version from last tag and increment
  old_version = get_last_version()
  info("Old version:", old_version)

  version = inc_version(old_version, args.feature, args.breakage)
  readable_version = version.replace("v", "")
  date = datetime.datetime.now().strftime("%Y-%m-%d")
  info("New version:", readable_version, version, date)

  # Disallow unstaged changes in the working tree
  run("git diff-files --quiet --ignore-submodules --")

  # Disallow uncommitted changes in the index
  run("git diff-index --cached --quiet HEAD --ignore-submodules --")

  # Update version string
  replace_in_file("README.md", old_version.replace("v", ""), readable_version)
  update_man("doc/tint2.md", readable_version, date)
  update_log("ChangeLog", readable_version, date)
  run("./update-generated.sh")
  run("git commit -am 'Release %s'" % readable_version)
  run("git tag -a %s -m 'version %s'" % (version, readable_version))
  run("git tag -a %s -m 'version %s'" % (readable_version, readable_version))
  run("rm -rf tint2-%s* || true" % readable_version)
  run("./make_release.sh")
  run("tar -xzf tint2-%s.tar.gz" % readable_version)
  run("cd tint2-%s ; mkdir build ; cd build ; cmake .. ; make" % readable_version)
  run("cd tint2-%s ; rm -r build" % readable_version)
  run("cd tint2-%s ; mkdir build ; cd build ; cmake -GNinja .. ; ninja" % readable_version)
  assert_equal(run("./tint2-%s/build/tint2 -v" % readable_version).strip(), "tint2 version %s" % readable_version)
  os.system("git log -p -1 --word-diff")

  print "Does this look correct? [y/n]"
  choice = raw_input().lower()
  if choice != "y":
    run("git reset --hard HEAD~ ; git tag -d %s ; git tag -d %s" % (version, readable_version))
    sys.exit(1)

  print "Publish? [y/n]"
  choice = raw_input().lower()
  if choice != "y":
    sys.exit(1)

  for origin in "origin-github", "origin-gitlab", "origin-opencode":
    run("git push " + origin + " master && git push --tags " + origin + " master")
