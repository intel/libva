# Contributing to libva

Libva is an open source project licensed under the [MIT License](https://opensource.org/licenses/MIT)

## Coding Style

Libva does not have a defined coding style at this time, but that will be updated.

## Certificate of Origin

In order to get a clear contribution chain of trust we use the [signed-off-by language](https://01.org/community/signed-process)
used by the Linux kernel project.  
## Patch format

Beside the signed-off-by footer, we expect each patch to comply with the following format:

```
<component>: Change summary

More detailed explanation of your changes: Why and how.
Wrap it to 72 characters.
See [here](http://chris.beams.io/posts/git-commit/)
for some more good advices.

Signed-off-by: <contributor@foo.com>
```

For example:

```
drm: remove va_drm_is_authenticated check
    
If we do not use a render node we must authenticate. Doing the extra
GetClient calls/ioctls does not help much, so don't bother.
    
Cc: David Herrmann <dh.herrmann@gmail.com>
Cc: Daniel Vetter <daniel.vetter@ffwll.ch>
Signed-off-by: Emil Velikov <emil.l.velikov@gmail.com>
Reviewed-by: Sean V Kelley <seanvk@posteo.de>
```

## Pull requests

We accept github pull requests.

Once you've finished making your changes push them to your fork and send the PR via the github UI.

## Issue tracking

If you have a problem, please let us know.  IRC is a perfectly fine place
to quickly informally bring something up, if you get a response.  The
[mailing list](https://lists.01.org/mailman/listinfo/intel-vaapi-media)
is a more durable communication channel.

If it's a bug not already documented, by all means please [open an
issue in github](https://github.com/01org/libva/issues/new) so we all get visibility
to the problem and can work towards a resolution.

For feature requests we're also using github issues, with the label
"enhancement".

Our github bug/enhancement backlog and work queue are tracked in a
[Libva waffle.io kanban](https://waffle.io/01org/libva).

## Closing issues

You can either close issues manually by adding the fixing commit SHA1 to the issue
comments or by adding the `Fixes` keyword to your commit message:

```
ssntp: test: Add Disconnection role checking tests

We check that we get the right role from the disconnection
notifier.

Fixes #121

Signed-off-by: Samuel Ortiz <sameo@linux.intel.com>
```

Github will then automatically close that issue when parsing the
[commit message](https://help.github.com/articles/closing-issues-via-commit-messages/).
