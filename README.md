# Home Appliance Controller (HAC) 🏠

A simple yet powerful client-server system for controlling home appliances, built in C with support for both IPv4 and IPv6 connections.

## Features ✨

- **Dual Stack Support**: Works with both IPv4 and IPv6 connections
- **Multi-threading**: Efficient server handling with separate threads for:
  - Client requests
  - File handling
  - Automated backups
- **Device Control**: Currently supports:
  - Light switches (On/Off)
  - Boiler control (On/Off)
- **State Persistence**: Maintains device states across restarts
- **Activity Logging**: Comprehensive logging system with:
  - Command history
  - State changes
  - Error tracking
- **Automated Backups**: Daily backups of all system logs and states

## Prerequisites 📋

- GCC Compiler
- POSIX-compliant system (Linux/Unix)
- pthread library
- Basic network connectivity

## Installation 🔧

1. Clone the repository:
```bash
git clone https://github.com/rudokir/hac.git
cd hac
```

2. Build the project:
```bash
chmod +x build.sh
./build.sh
```

## Usage 💡

### Starting the Server

```bash
./Server
```

### Using the Client

Connect to localhost (default):
```bash
./User
```

Connect to specific host:
```bash
./User hostname
```

Connect and send command:
```bash
./User hostname command
```

### Available Commands

| Command | Description |
|---------|-------------|
| b_on    | Turn boiler on |
| b_off   | Turn boiler off |
| sw_on   | Turn light on |
| sw_off  | Turn light off |

## System Architecture 🏗️

### Server Components

- **Client Request Handler**: Manages incoming connections and command processing
- **File Handler**: Handles state persistence and logging
- **Backup System**: Performs daily backups at 3:00 AM

### File Structure

```
├── Database Files
│   ├── database.txt          # Main log file
│   ├── light.txt            # Light state
│   ├── boiler.txt           # Boiler state
│   ├── sw_light.txt         # Light switch history
│   └── sw_boiler.txt        # Boiler switch history
├── Source Files
│   ├── SmartHomeServer.c    # Server implementation
│   └── User.c               # Client implementation
└── build.sh                 # Build script
```

## Safety Features 🔒

- **Error Handling**: Comprehensive error checking and reporting
- **Buffer Overflow Protection**: Safe string handling and buffer size checking
- **State Validation**: Prevents invalid state transitions
- **Connection Management**: Proper handling of connection failures and timeouts

## Contributing 🤝

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Known Issues 🐛

- No authentication system implemented yet
- Limited to binary state devices (on/off)
- Backup system requires sufficient disk space

## Future Enhancements 🚀

- [ ] Add user authentication
- [ ] Support for analog devices (dimmers, thermostats)
- [ ] Web interface
- [ ] Mobile app integration
- [ ] MQTT support
- [ ] SSL/TLS encryption
- [ ] Configuration file support
- [ ] Device auto-discovery

## License 📄

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments 👏

- Inspired by the need for a lightweight home automation solution
- Built with security and reliability in mind
- Designed for extensibility and ease of use

---

Feel free to contribute to this project or report any issues you encounter! 🌟
