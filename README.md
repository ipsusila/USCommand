# USCommand

Universal String command for Arduino.

## Command format

Command mimics URL format and can be used to control multiple board connected e.g. using RS485.

```
!aaa[.aaa.aaa.aaa][:mmmmm][/action/action][?param=key&param=key&param][|checksum]$
```

Example commands:

```
!1$
  !2$
!1:01$
!2.3$
    !0.0.2.3:123|120$   
!0.0.1\:123$
@123:100/1234$
!0.0.1.2:100/test/abcde$
!0.0.1.2:100/test/abcde?start$
!0.0.1.2:100/test/abcde?t=1$
!0.0.1.2:100/test/abcde?t=1&e=2|7$
!0.0.1.2:100/test/abcde?t=1&s$
```

### Definitions

Definitions of each fields:

1. `!` denotes beginning of a command, while `@` denotes beginning of response
2. `aaa[.aaa.aaa.aaa]` denotes device address
3. `:mmmmm` denotes module address
4. `action/action` denotes action
5. `param=key&param=key&param` denotes list of parameters and optionally its values
6. `|checksum` denotes checksum calculated by XORing all characters from `!` to before `|`
7. `$` denotes end of a command.

### Special characters

Special characters includes: `\\`, `\r`, `\n`, `\t`, `\b`, `\$`, `\&`, `\=`, `\|` and only allowed in the param value.
