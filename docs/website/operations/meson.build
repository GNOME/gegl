
if not can_run_host_binaries
  warning('Cannot create operation gallery in this environment')
  subdir_done()
endif

subdir('images')

# Generate operation html files.
doc_ops_html_cmd = [
  find_program(project_source_root / 'tools' / 'run-exe.py'),
  '--output-dir', '@OUTDIR@',
  operations_html,
]

doc_operations_html = custom_target('doc_operations_html',
  env: doc_env,
  output: 'index.html',
  command: [doc_ops_html_cmd],
  depends: doc_operations_img,
  build_by_default: true,
)
