param(
    [Parameter()]
    [String]$architecture
)

function EnterDevShellEnv  {

    param(
        [Parameter()]
        [String]$arch
    )

    $vsw = Get-Command 'vswhere'
    $VSFfavors = 'Community','Professional','Enterprise','BuildTools' | %{ "Microsoft.VisualStudio.Product.$_" }
    $vs = & $vsw.Path -products $VSFfavors -latest -format json | ConvertFrom-Json
    $tools_dir = Join-Path $vs.installationPath 'Common7' 'Tools'
    # Try the root tools dir
    $devshell = Join-Path $tools_dir 'Microsoft.VisualStudio.DevShell.dll'
    # Try finding it under vsdevshell
    if (!(Test-Path $devshell -Type Leaf)) {
        $devshell = Join-Path $tools_dir 'vsdevshell' 'Microsoft.VisualStudio.DevShell.dll'
    }
    # Fail if didn't find the DevShell library
    if (!(Test-Path $devshell -Type Leaf)) {
        throw "error: cannot find Microsoft.VisualStudio.DevShell.dll"
    }
    Import-Module $devshell
    Enter-VsDevShell -VsInstanceId $vs.instanceId -SkipAutomaticLocation -DevCmdArguments "-arch=$arch -no_logo"
}

# Enter VsDevShell, capture the environment difference and export it to github env
$env_before = @{}
Get-ChildItem env: | %{ $env_before.Add($_.Name, $_.Value) }
EnterDevShellEnv -arch "$architecture"
$env_after = @{}
Get-ChildItem env: | %{ $env_after.Add($_.Name, $_.Value) }
$env_diff = $env_after.GetEnumerator() | where { -not $env_before.ContainsKey($_.Name) -or $env_before[$_.Name] -ne $_.Value }
$env_diff | %{ echo "$($_.Name)=$($_.Value)" >> $env:GITHUB_ENV }
