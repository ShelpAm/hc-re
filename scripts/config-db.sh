check-env() {
    varname=$1
    if [ -n "${!varname}" ]; then
        echo "Missing environment variable ${varname}, aborting"
        exit 1
    fi
}

check-env HCRE_DB_NAME
check-env HCRE_DB_PASSWORD

## TODO 不安全，用文件存敏感信息比较好(？) 请切换方式！
