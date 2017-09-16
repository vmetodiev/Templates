#!/usr/bin/python

from ansible.module_utils.basic import *


def main():

    module = AnsibleModule(argument_spec={})
    cmd_rc, cmd_stdout, cmd_stderr = module.run_command("date")
    module.exit_json(changed=False, meta=cmd_stdout)

if __name__ == '__main__':
    main()
