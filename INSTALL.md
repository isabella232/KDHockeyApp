Installation Instructions
=========================

KD HockeyApp requires a Qt version >= 5.2 with Network and QML
support enabled.  To build Google breakpad in 3rdparty you'll
also need the GNU autoconf tools.

1. From the top directory of your extracted KD HockeyApp package
   run qmake to configure.

   Pick the build type like this:

       qmake CONFIG+=debug
       qmake CONFIG+=release

   To define the install location use standard directory variables,
   e.g:

       qmake PREFIX=/opt/kdhockeyapp
       qmake LIBDIR=/usr/lib64

   If this succeeded, jump to step 2.

2. Build everything by typing:

       make             # Unix
       nmake            # Windows

3. Optionally install KD HockeyApp:

        make install    # Unix
        nmake install   # Windows

   To specify a temporary install location set the INSTALL_ROOT
   variable:

        make install INSTALL_ROOT=/tmp/kdhockeyapp   # Unix

4. Have a look at the examples applications. They will get you started
   with KD HockeyApp.
