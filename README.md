# Coda signature app for Ledger Blue & Ledger Nano S/X 

Big disclaimer : This is a work in progress. Don't use it on a Ledger device 
that is handling or has ever handled private keys associated with any accounts
of value.

This app grew loosely from the Ledger samplesign app, but all of the python in `/cli` 
has been updated to use python3. So if you're here from another project and curious,
it seems it is possible!

Run `make` to check the build, and `make load` to build and load the application 
onto the device. Errors try to be helpful and the most common reason for failure 
is the device autolocking, so if something isn't working that might just be the 
reason. After installing and running the application, you can run 
`python3 cli/sign.py` to test a signature over USB. `make delete` deletes the app.

See [Ledger's documentation](http://ledger.readthedocs.io) for further information.