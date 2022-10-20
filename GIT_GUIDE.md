Just a quick run-down on git if you guys need. You can clone a repository (download it by running a command like this). Make sure you do this in the directory (folder) that you want. Make sure git is installed too.

```bash
git clone https://github.com/orosmatthew/memory-cache-server
```

Then navigate to the directory. All the `git` command have to be run from the folder that contains the repository or they won't work.

```bash
cd memory-cache-server
```

You can change whatever files you want, this is a local copy. To add and push changes you first stage/add the files you want to commit using this command. Only add files that are necessary to be changed, don't add temporary files that shouldn't be part of the repo like output files from programs.

```bash
git add ./file1 ./src/file2 ./file3 ./etc
```

After you add the files you want, you commit it.

```bash
git commit -m "Type a short message here about what you changed"
```

Once committed, you have to push the changes to the remote origin which is GitHub. You can do this with this command.

```bash
git push
```

If you get an error on the push command, you might not have push privileges to the repo so let me know if that happens.

If you need to update/sync your local copy with the remote/origin version on GitHub, you do this. Before doing any major changes, I would always do a `git pull` to get all changes anyone else has made to the repo.

```bash
git pull
```

If you get an error, there might be local changes that conflict with the remote version so you can reset your local changes by running these commands. But this will delete local changes so you might want to back them up somewhere if you want to keep them. 

```bash
git reset --hard
git pull
```
