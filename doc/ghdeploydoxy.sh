#!/bin/bash
################################################################################
# Notes         :
# Preconditions:
# - Packages doxygen doxygen-doc doxygen-latex doxygen-gui graphviz
#   must be installed.
# - Doxygen configuration file must have the destination directory empty and
#   source code directory.
# - A gh-pages branch should already exist.
#
# Required global variables:
# - DOXYFILE            : The Doxygen configuration file.
# - GH_REPO_NAME        : The name of the repository.
# - GH_REPO_REF         : The GitHub reference to the repository.
# - GH_REPO_TOKEN       : The GitHub application token.
#
# This script will generate Doxygen documentation and push the documentation to
# the gh-pages branch of a repository specified by GH_REPO_REF.
# Before this script is used there should already be a gh-pages branch in the
# repository.
#
################################################################################

################################################################################
##### Setup this script and get the current gh-pages branch.               #####
echo 'Setting up the script...'
# Exit with nonzero exit code if anything fails
set -e

GH_REPO_NAME=
GH_REPO_REF=
GH_REPO_TOKEN=

usage() { echo "Usage: `basename $0` options (-n value) (-r value) (-t value)" 1>&2; exit 1; }

if ( ! getopts ":n:r:t:" opt); then
      usage
fi

while getopts :n:r:t: opt; do
  case $opt in
  n)
      GH_REPO_NAME=$OPTARG
      ;;
  r)
      GH_REPO_REF=$OPTARG
      ;;
  t)
      GH_REPO_TOKEN=$OPTARG
      ;;
  *)
      usage
      ;;
  esac
done

shift $((OPTIND - 1))

[ -n "$GH_REPO_NAME" ] || {
    echo "ERROR: -n GH_REPO_NAME is not defined" >/dev/stderr
    exit 1
}

[ -n "$GH_REPO_REF" ] || {
    echo "ERROR: -r GH_REPO_REF is not defined" >/dev/stderr
    exit 1
}

[ -n "$GH_REPO_TOKEN" ] || {
    echo "ERROR: -t GH_REPO_TOKEN is not defined" >/dev/stderr
    exit 1
}

################################################################################
##### Upload the documentation to the gh-pages branch of the repository.   #####
# Only upload if Doxygen successfully created the documentation.
# Check this by verifying that the html directory and the file html/index.html
# both exist. This is a good indication that Doxygen did it's work.
if [ -d "html-out" ] && [ -f "html-out/index.html" ]; then

    # Create a clean working directory for this script.
    mkdir code_docs
    cd code_docs

    # Get the current gh-pages branch
    git clone -b gh-pages https://git@$GH_REPO_REF
    cd $GH_REPO_NAME

    ##### Configure git.
    # Set the push default to simple i.e. push only the current branch.
    git config --global push.default simple

    # Remove everything currently in the gh-pages branch.
    # GitHub is smart enough to know which files have changed and which files have
    # stayed the same and will only update the changed files. So the gh-pages branch
    # can be safely cleaned, and it is sure that everything pushed later is the new
    # documentation.
    CURRENTCOMMIT=`git rev-parse HEAD`
    git reset --hard `git rev-list HEAD | tail -n 1` # Reset working tree to initial commit
    git reset --soft $CURRENTCOMMIT # Move HEAD back to where it was

    # Move doxy files into local gh-pages branch folder
    mv ../../html-out/* .

    # Need to create a .nojekyll file to allow filenames starting with an underscore
    # to be seen on the gh-pages site. Therefore creating an empty .nojekyll file.
    # Presumably this is only needed when the SHORT_NAMES option in Doxygen is set
    # to NO, which it is by default. So creating the file just in case.
    echo "" > .nojekyll

    echo 'Uploading documentation to the gh-pages branch...'
    # Add everything in this directory (the Doxygen code documentation) to the
    # gh-pages branch.
    # GitHub is smart enough to know which files have changed and which files have
    # stayed the same and will only update the changed files.
    git add --all

    # Commit the added files with a title and description containing the Travis CI
    # build number and the GitHub commit reference that issued this build.
    git commit -m "Deploy code docs to GitHub Pages"

    # Force push to the remote gh-pages branch.
    # The ouput is redirected to /dev/null to hide any sensitive credential data
    # that might otherwise be exposed.
    git push --force "https://${GH_REPO_TOKEN}@${GH_REPO_REF}" > /dev/null 2>&1
else
    echo '' >&2
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi
