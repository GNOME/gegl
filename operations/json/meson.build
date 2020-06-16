
json_operations = [
  'grey2.json',
  'dropshadow2.json'
]

# copy json files into build so they are available for tests/docs
foreach _file : json_operations
  json_operations += configure_file(
    input: _file,
    output: _file,
    copy: true,
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endforeach
