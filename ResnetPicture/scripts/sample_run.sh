#!/bin/bash
ScriptPath="$( cd "$(dirname "$BASH_SOURCE")" ; pwd -P )"

function main()
{
  echo "[INFO] The sample starts to run"
  running_command="./main"
  cd ${ScriptPath}/../out
  ${running_command}
  if [ $? -ne 0 ];then
      echo "[INFO] The program runs failed"
  else
      echo "[INFO] The program runs successfully"
  fi
}
main
