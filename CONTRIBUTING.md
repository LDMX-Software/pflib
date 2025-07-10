# Contributing

**All Contributes are Welcome**
From fixing typos in these documentation pages to patching a bug the code to adding a feature.
Please reach out via GitHub issues or on the LDMX slack to get started.

To contribute code to the project, you will need to create an account on [github](https://github.com/) if you don't have one already, and then request to be added to the [LDMX-Software](https://github.com/orgs/LDMX-Software/) organization (@tomeichlersmith can add you if you send him your GitHub user name).

When adding new code, you should do this on a branch created by a command like
`git switch -c <branch-name>` in order to make sure you don't apply changes directly to the main branch.
We typically create branches based on issue names in the github bug tracker,
so "Issue 1234: Short Description in Title" turns into the branch name `1234-short-desc`.
This is the same branch name format that is created if you ask GitHub to create a branch for you.

**Note** :warning: Make sure to update your local copy of `main` _before_ creating your new branch.
```
git switch main
git pull origin main
git switch -c <branch-name> # creating new branch
```
This avoids conflicts with changes other folks have made in the mean time.
Get in the habit of updating `main` regularly so you can get other folks' fixes and features!

Then you would `git add` and `git commit` your changes to this branch.

If you don't already have SSH keys configured, look at the [GitHub directions](https://help.github.com/en/github/authenticating-to-github/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent). This makes it easier to push/pull to/from branches on GitHub!

## Pull Requests

We prefer that any code contributions are submitted via [pull requests](https://help.github.com/articles/creating-a-pull-request/) so that they can be reviewed before changes are merged into the main branch.

Before you start, an [issue should be added to the pflib issue tracker](https://github.com/LDMX-Software/pflib/issues/new).

### Branch Name Convention
Then you should make a local branch from `trunk` using a command like `git checkout -b 1234-short-desc` where _1234_ is the issue number from the issue tracker and `short-desc` is a short description (using `-` as spaces) of what the branch is working one.

Once you have committed your local changes to this branch using the `git add` and `git commit` commands, then push your branch to github using a command like `git push -u origin 1234-short-desc`.

Finally, [submit a pull request](https://github.com/LDMX-Software/pflib/compare) to integrate your changes by selecting your branch in the _compare_ dropdown box and clicking the green buttons to make the PR.  This should be reviewed and merged or changes may be requested before the code can be integrated into the master.

If you plan on starting a major (sub)project within the repository like adding a new code module, you should give advance notice and explain your plains beforehand. :) A good way to do this is to create a new issue. This allows the rest of the code development team to see what your plan is and offer comments/questions.
