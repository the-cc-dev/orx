REBOL [
    title: {Init}
    author: {iarwain@orx-project.org}
    date: 15-Aug-2017
    file: %init.r
]

; Variables
source: %../template/
template: {[orx]}
extern: %../../../extern/
params: reduce [
    'name           {Project name, relative or full path}   _
]
platforms:  [
    {windows}   [config [{gmake} {codelite} {vs2013} {vs2015} {vs2017}]     premake %premake4.exe   setup {setup.bat}   script %init.bat    ]
    {mac}       [config [{gmake} {codelite} {xcode4}                  ]     premake %premake4       setup {./setup.sh}  script %./init.sh   ]
    {linux}     [config [{gmake} {codelite}                           ]     premake %premake4       setup {./setup.sh}  script %./init.sh   ]
]

; Helpers
delete-dir: func [
    {Deletes a directory including all files and subdirectories.}
    dir [file! url!]
] [
    if all [
        dir? dir
        dir: dirize dir
        attempt [files: load dir]
    ] [
        foreach file files [delete-dir dir/:file]
    ]
    attempt [delete dir]
]
log: func [
    message [string! block!]
    /only
    /no-break
] [
    unless only [
        prin [{[} now/time {] }]
    ]
    either no-break [prin message] [print/eval message]
]

; Inits
switch platform: lowercase to-string system/platform/1 [
    {macintosh} [platform: {mac}]
]
platform-info: platforms/:platform
premake-source: rejoin [%../ platform-info/premake]
change-dir root: system/options/path

; Usage
usage: func [
    /message content [block! string!]
] [
    if message [
        prin {== }
        print/eval content
        print {}
    ]

    prin [{== Usage:} to-local-file clean-path rejoin [system/options/script/../../../.. "/" platform-info/script]]

    print rejoin [
        newline newline
        map-each [param desc default] params [
            prin rejoin [{ } either default [rejoin [{[} param {]}]] [param]]
            rejoin [{  = } param {: } desc either default [rejoin [{=[} default {], optional}]] [{, required}] newline]
        ]
    ]
    quit
]

; Processes params
either system/options/args [
    either (length? system/options/args) > ((length? params) / 3) [
        usage/message [{Too many arguments:} mold system/options/args]
    ] [
        use [arg] [
            arg: system/options/args
            foreach [param desc default] params [
                either tail? arg [
                    either default [
                        set param default
                    ] [
                        usage/message [{Not enough arguments:} mold system/options/args]
                    ]
                ] [
                    set param arg/1
                    arg: next arg
                ]
            ]
        ]
    ]
] [
    usage
]

; Locates source
change-dir rejoin [first split-path system/options/script source]
source: what-dir
change-dir root

; Runs setup if premake isn't found
unless exists? source/:premake-source [
    log [{New orx installation found, running setup!}]
    attempt [delete-dir source/:extern]
    change-dir source/../../..
    call/shell/wait platform-info/setup
    change-dir root
]

; Inits project directory
if exists? name: to-rebol-file name [
    log [{[} to-local-file name {] already exists, erasing!}]
    delete-dir dirize name
]
make-dir/deep name
change-dir name/..
name: second split-path name
log [{Initializing [} name {] in [} to-local-file what-dir {]}]

; Copies all files
log {== Creating files:}
build: _
eval copy-files: func [
    from [file!]
    to [file!]
    /local src dst
] [
    foreach file read from [
        src: from/:file
        dst: replace to/:file template name
        if file = %build/ [
            set 'build dst
        ]
        either dir? src [
            make-dir/deep dst
            copy-files src dst
        ] [
            log/only [{  +} to-local-file dst]
            write dst replace/all read src template name
        ]
    ]

] source name

; Creates build projects
if build [
    change-dir build
    write platform-info/premake read source/:premake-source
    unless platform = {windows} [
        call/shell/wait reform [{chmod +x} platform-info/premake]
    ]
    log [{Generating build files for [} platform {]:}]
    foreach config platform-info/config [
        log/only [{  *} config]
        call/shell/wait reform [to-local-file clean-path platform-info/premake config]
    ]
]

; Ends
log {Init successful!}
