install_subdir(
    'icons',
    install_dir: share_dir,
)

install_data(
    '../scripts/apply-brightness.sh',
    install_dir: get_option('libexecdir') / 'wfplug-brightness',
    install_mode: 'rwxr-xr-x',
    rename: 'apply-brightness',
)

install_data(
    'wfplug-brightness@.service',
    install_dir: '/lib/systemd/system',
)
