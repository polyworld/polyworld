# Markdown Contents
A snapshot of the wiki contents is contained within your local repository at `docs/wiki/md/*.md`, where the content is in the form of Github Flavored Markdown (GFM). The GFM format is plaintext, so it can be viewed from within any text editor. If you do use a text editor to view the GFM, you'll probably want to enable line wrapping.

# Viewing as HTML
The GFM files can be converted into HTML and viewed from your browser. The conversion process relies on Github servers, so it requires internet connectivity. Once the conversion is complete, though, you can view the HTML from your browser without an internet connection.

## Install Converter (Linux)
The `flavor` converter requires ruby, ruby development files, and the `json` gem. On a Debian-like system, ruby can be installed via:
```
sudo apt-get install ruby ruby-dev
```

Next, install the `json` gem:
```
sudo gem install json
```

## Install Converter (Mac)
The `flavor` converter requires ruby and the `json` gem. Ruby should already be installed on your system, but you may need to install the `json` gem via:
```
sudo gem install json
```

## Convert GFM to HTML
*The remaining instructions assume that you have installed Polyworld to the directory `/home/fred/polyworld`. You'll need to change the commands to use the correct directory name.*

Move into the Polyworld directory:
```
cd /home/fred/polyworld
```

Convert the files:
```
./scripts/wiki/mkhtml
```

You should now see a directory `./docs/wiki/html` that is filled with files with no extension -- the `*.html` extension is not used so that hyperlinks within the files will continue to work locally.

## Open in Your Browser
You should now be able to enter a URL similar to the following in your web browser's address bar (substituting the correct installation directory):
```
file:///home/fred/polyworld/docs/wiki/html/Home
```

# Downloading a Local Wiki Snapshot
To download the wiki state for your local release state:
```
./scripts/wiki/checkout
```

To download the latest online wiki state:
```
./scripts/wiki/pull
```
Note that the pull will result in a merge operation, creating a commit. This is mostly intended for admins.
