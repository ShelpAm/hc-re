# Homework Collection - Remastered

Due to unfamiliar with golang, I ran into mess of code. In this remaster, I want to reorganize and
write clean code in cpp.

## Features

- Complete flow of assignment submission, management and grading.
- Support computing AIGC rate of submitted assignment.
- Support plagiarism checking for assignments in a same assignment.
- Pretty visual graphs for teachers and students to check the status of
  assignment submission and grading.

## Prerequisite

You should install `cmake`, `perl`.

C++ dependencies are managed by `conan`. You can install `conan` by pip:
```bash
pip install conan
```

Then initialize `conan` by:
```bash
conan profile detect
```

You may use `conan` to install required c++ packages. If you do, under project
root, run:
```bash
conan install . -of build -b missing
```

## Quickstart

```bash
cmake --preset conan-default # In some installation, use `conan-release`
cmake --build build
./build/apps/cli/hc
```

## Configuration

You can override configurations by putting your configuration in
`${XDG_CONFIG_HOME}/hc/config.yaml` or with `yml` extension. And the following
is default configuration:
```yaml
# HC-RE default configuration
# All fields are optional and will fall back to their default values

port: 8080  # uint16 - HTTP server port (0-65535)

db:
  host: "localhost"      # string - PostgreSQL server hostname or IP
  name: "hc"             # string - Database name
  user: "postgres"       # string - Database username
  password: ""           # string - Database password
```

## Contributing

If you want to contribute to this project, you may need to read [DEVELOP.md](/DEVELOP.md) first to
get knowledge of tricky part in development.
