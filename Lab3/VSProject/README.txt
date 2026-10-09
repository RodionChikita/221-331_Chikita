Purpose of SampleEnclave:
=========
The project demonstrates several fundamental usages:
    1. Initializing and destroying an enclave
    2. Creating ECALLs or OCALLs
    3. Calling trusted libraries inside the enclave

How to build/execution:
=========
Windows:
	1. Make sure you have the Intel® Software Guard Extensions Windows SDK package installed.
	2. Enclave test key(two options):
    		a. The project will generate a test key<Enclave_private.pem> automatically when you build the project.
    		b. Rename your test key(3072-bit RSA private key) to <Enclave_private.pem> and put it under the <Enclave> folder.
	3. Open the Visual Studio solution file "SampleEnclave.sln". 
	4. Build and execute it directly.
Linux:
	1. Make sure you have the Intel® Software Guard Extensions Linux SDK package installed.
	2. Enclave test key(two options):
		a. Install openssl first, then the project will generate a test key<Enclave_private_test.pem> automatically when you build the project.
    		b. Rename your test key(3072-bit RSA private key) to <Enclave_private_test.pem> and put it under the <Enclave> folder.

	3. Build the project with the prepared Makefile:
		a. Hardware Mode, Debug build:
		    $ make SGX_MODE=HW SGX_DEBUG=1
		b. Hardware Mode, Pre-release build:
		    $ make SGX_MODE=HW SGX_PRERELEASE=1
		c. Hardware Mode, Release build:
		    $ make SGX_MODE=HW
		d. Simulation Mode, Debug build:
		    $ make SGX_DEBUG=1
		e. Simulation Mode, Pre-release build:
		    $ make SGX_PRERELEASE=1
		f. Simulation Mode, Release build:
		    $ make
	4. Execute the binary directly:
		$ ./app

