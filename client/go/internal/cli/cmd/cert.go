// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// vespa cert command
// Author: mpolden
package cmd

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/fatih/color"
	"github.com/spf13/cobra"
	"github.com/vespa-engine/vespa/client/go/internal/util"
	"github.com/vespa-engine/vespa/client/go/internal/vespa"
)

func newCertCmd(cli *CLI) *cobra.Command {
	var (
		skipApplicationPackage bool
		overwriteCertificate   bool
	)
	cmd := &cobra.Command{
		Use:   "cert",
		Short: "Create a new private key and self-signed certificate for data-plane access with Vespa Cloud",
		Long: `Create a new private key and self-signed certificate for data-plane access with Vespa Cloud.

The private key and certificate will be stored in the Vespa CLI home directory
(see 'vespa help config'). Other commands will then automatically load the
certificate as necessary. The certificate will be added to your application
package specified as an argument to this command (default '.').

It's possible to override the private key and certificate used through
environment variables. This can be useful in continuous integration systems.

It's also possible override the CA certificate which can be useful when using self-signed certificates with a
self-hosted Vespa service. See https://docs.vespa.ai/en/mtls.html for more information.

Example of setting the CA certificate, certificate and key in-line:

    export VESPA_CLI_DATA_PLANE_CA_CERT="my CA cert"
    export VESPA_CLI_DATA_PLANE_CERT="my cert"
    export VESPA_CLI_DATA_PLANE_KEY="my private key"

Example of loading CA certificate, certificate and key from custom paths:

    export VESPA_CLI_DATA_PLANE_CA_CERT_FILE=/path/to/cacert
    export VESPA_CLI_DATA_PLANE_CERT_FILE=/path/to/cert
    export VESPA_CLI_DATA_PLANE_KEY_FILE=/path/to/key

Note that when overriding key pair through environment variables, that key pair
will always be used for all applications. It's not possible to specify an
application-specific key.

Read more in https://cloud.vespa.ai/en/security/guide`,
		Example: `$ vespa auth cert -a my-tenant.my-app.my-instance
$ vespa auth cert -a my-tenant.my-app.my-instance path/to/application/package`,
		DisableAutoGenTag: true,
		SilenceUsage:      true,
		Args:              cobra.MaximumNArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return doCert(cli, overwriteCertificate, skipApplicationPackage, args)
		},
	}
	cmd.Flags().BoolVarP(&overwriteCertificate, "force", "f", false, "Force overwrite of existing certificate and private key")
	// TODO(mpolden): Stop adding certificate to application package and remove this flag
	cmd.Flags().BoolVarP(&skipApplicationPackage, "no-add", "N", false, "Do not add certificate to the application package")
	cmd.MarkPersistentFlagRequired(applicationFlag)
	return cmd
}

func newCertAddCmd(cli *CLI) *cobra.Command {
	var overwriteCertificate bool
	cmd := &cobra.Command{
		Use:   "add",
		Short: "Add certificate to application package",
		Long: `Add an existing self-signed certificate for Vespa Cloud deployment to your application package.

The certificate will be loaded from the Vespa CLI home directory (see 'vespa
help config') by default.

The location of the application package can be specified as an argument to this
command (default '.').`,
		Example: `$ vespa auth cert add -a my-tenant.my-app.my-instance
$ vespa auth cert add -a my-tenant.my-app.my-instance path/to/application/package`,
		DisableAutoGenTag: true,
		SilenceUsage:      true,
		Args:              cobra.MaximumNArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return doCertAdd(cli, overwriteCertificate, args)
		},
	}
	cmd.Flags().BoolVarP(&overwriteCertificate, "force", "f", false, "Force overwrite of existing certificate")
	cmd.MarkPersistentFlagRequired(applicationFlag)
	return cmd
}

func doCert(cli *CLI, overwriteCertificate, skipApplicationPackage bool, args []string) error {
	app, err := cli.config.application()
	if err != nil {
		return err
	}
	targetType, err := cli.targetType()
	if err != nil {
		return err
	}
	privateKeyFile, err := cli.config.privateKeyPath(app, targetType.name)
	if err != nil {
		return err
	}
	certificateFile, err := cli.config.certificatePath(app, targetType.name)
	if err != nil {
		return err
	}

	if !overwriteCertificate {
		hint := "Use -f flag to force overwriting"
		if util.PathExists(privateKeyFile.path) {
			return errHint(fmt.Errorf("private key %s already exists", color.CyanString(privateKeyFile.path)), hint)
		}
		if util.PathExists(certificateFile.path) {
			return errHint(fmt.Errorf("certificate %s already exists", color.CyanString(certificateFile.path)), hint)
		}
	}

	keyPair, err := vespa.CreateKeyPair()
	if err != nil {
		return err
	}
	if err := keyPair.WriteCertificateFile(certificateFile.path, overwriteCertificate); err != nil {
		return fmt.Errorf("could not write certificate: %w", err)
	}
	if err := keyPair.WritePrivateKeyFile(privateKeyFile.path, overwriteCertificate); err != nil {
		return fmt.Errorf("could not write private key: %w", err)
	}
	cli.printSuccess("Certificate written to ", color.CyanString(certificateFile.path))
	cli.printSuccess("Private key written to ", color.CyanString(privateKeyFile.path))
	if !skipApplicationPackage {
		return doCertAdd(cli, overwriteCertificate, args)
	}
	return nil
}

func doCertAdd(cli *CLI, overwriteCertificate bool, args []string) error {
	pkg, err := cli.applicationPackageFrom(args, false)
	if err != nil {
		return err
	}
	target, err := cli.target(targetOptions{})
	if err != nil {
		return err
	}
	if pkg.HasCertificate() && !overwriteCertificate {
		return errHint(fmt.Errorf("application package %s already contains a certificate", pkg.Path), "Use -f flag to force overwriting")
	}
	return maybeCopyCertificate(true, false, cli, target, pkg)
}

func maybeCopyCertificate(force, ignoreZip bool, cli *CLI, target vespa.Target, pkg vespa.ApplicationPackage) error {
	if pkg.IsZip() && !ignoreZip {
		hint := "Try running 'mvn clean', then 'vespa auth cert add' and finally 'mvn package'"
		return errHint(fmt.Errorf("cannot add certificate to compressed application package: %s", pkg.Path), hint)
	}
	if force {
		return copyCertificate(cli, target, pkg)
	}
	if pkg.HasCertificate() {
		return nil
	}
	if cli.isTerminal() {
		cli.printWarning("Application package does not contain " + color.CyanString("security/clients.pem") + ", which is required for deployments to Vespa Cloud")
		ok, err := cli.confirm("Do you want to copy the certificate of application "+color.GreenString(target.Deployment().Application.String())+" into this application package?", true)
		if err != nil {
			return err
		}
		if ok {
			return copyCertificate(cli, target, pkg)
		}
	}
	return errHint(fmt.Errorf("deployment to Vespa Cloud requires certificate in application package"),
		"See https://cloud.vespa.ai/en/security/guide",
		"Pass --add-cert to use the certificate of the current application")
}

func copyCertificate(cli *CLI, target vespa.Target, pkg vespa.ApplicationPackage) error {
	tlsOptions, err := cli.config.readTLSOptions(target.Deployment().Application, target.Type())
	if err != nil {
		return err
	}
	hint := "Try generating the certificate with 'vespa auth cert'"
	if tlsOptions.CertificateFile == "" {
		return errHint(fmt.Errorf("no certificate exists for "+target.Deployment().Application.String()), hint)
	}
	data, err := os.ReadFile(tlsOptions.CertificateFile)
	if err != nil {
		return errHint(fmt.Errorf("could not read certificate file: %w", err))
	}
	dstPath := filepath.Join(pkg.Path, "security", "clients.pem")
	if err := os.MkdirAll(filepath.Dir(dstPath), 0755); err != nil {
		return fmt.Errorf("could not create security directory: %w", err)
	}
	err = util.AtomicWriteFile(dstPath, data)
	if err == nil {
		cli.printSuccess("Copied certificate from ", tlsOptions.CertificateFile, " to ", dstPath)
	}
	return err
}
