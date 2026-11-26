### thinkfancpp

simple Qt6 ThinkPad fan controller.

### Requires

- A **ThinkPad**
- Any Linux distro

### How it works

Controls `/proc/acpi/ibm/fan` through `pkexec` + `thinkfancpp-helper`.

### Setup

Create a modprobe config file with vim or any:

```bash
doas vim /etc/modprobe.d/thinkpad_acpi.conf
```

Add this line, and save:

```bash
options thinkpad_acpi fan_control=1 
```

Or write it non-interactively:

```bash
printf 'options thinkpad_acpi fan_control=1\n' | sudo tee /etc/modprobe.d/thinkpad_acpi.conf
```

Reload the module:

```bash
sudo modprobe -r thinkpad_acpi && sudo modprobe thinkpad_acpi
cat /sys/module/thinkpad_acpi/parameters/fan_control  # expect Y
```

### Build / test / run
```bash
bash build.sh 
```

### Install
```bash
bash build.sh --install
```

### Uninstall
```bash
bash build.sh --Uninstall
```

### Note

- **thinkfan conflict:** The app warns when `thinkfan` units are active. Do not run two fan writers at once. Stop thinkfan first if needed (`sudo systemctl stop thinkfan`) or remove it.
- **Disengaged mode:** Use at your own risk. The fan control state where the embedded controller (EC) disables closed-loop speed regulation and drives the fan using open-loop control at maximum power.
