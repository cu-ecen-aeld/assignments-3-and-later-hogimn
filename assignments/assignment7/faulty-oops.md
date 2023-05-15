# faulty-oops

## Validation

You should be able to clone your final buildroot assignment repository to a new directory, run ./build.sh to build the system image and ./runqemu.sh to start the image with no other interaction necessary.

#### a. The image should include printk logs in /var/log/messages including your github user name from the hello module which is loaded automatically during startup.

![a](https://github.com/cu-ecen-aeld/assignments-3-and-later-hogimn/assets/110673139/ccc6023a-d203-4f33-a603-89d540525cc5)

#### b. You should should have /dev/scull devices in the /dev directory as well as /dev/faulty

![b](https://github.com/cu-ecen-aeld/assignments-3-and-later-hogimn/assets/110673139/33d64d42-e211-442b-8cef-44c28c854b79)

#### c. Modules hello, faulty and scull should all be listed in lsmod after startup.

![c](https://github.com/cu-ecen-aeld/assignments-3-and-later-hogimn/assets/110673139/faa29f9f-314e-442e-8d73-9a037845f127)

#### d. Running /etc/init.d/S98lddmodules stop should unload all modules on the buildroot instance.

![d](https://github.com/cu-ecen-aeld/assignments-3-and-later-hogimn/assets/110673139/894fb045-51ee-4417-8eae-b5a6c77fe6aa)

#### e. Your analysis in your assignment 3 repository within assignments/assignment7/faulty-oops.md should explain the content and how you can use this to locate the faulty line in the kernel driver.

![e](https://github.com/cu-ecen-aeld/assignments-3-and-later-hogimn/assets/110673139/faaa918b-0889-43b5-9424-e9bfa13ad260)

1. It is Null pointer exception.
```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
```

2. Locate the position where throws exception.
We can find the location from pc register
```assembly
pc : faulty_write+0x14/0x20 [faulty]
```

or from call stack.
```assembly
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x40/0xa0
 el0_svc+0x20/0x60
 el0t_64_sync_handler+0xe8/0xf0
 el0t_64_sync+0x1a0/0x1a4
```

3. See the assembly code using objdump

![f](https://github.com/cu-ecen-aeld/assignments-3-and-later-hogimn/assets/110673139/737f12ac-6350-453a-b65a-17435b83cd44)

```assembly
14:	b900003f 	str	wzr, [x1]
```

This line is the cause of the crash.

