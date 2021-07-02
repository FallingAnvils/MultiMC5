Name:           MultiServerMC5
Version:        1.4
Release:        1%{?dist}
Summary:        A local install wrapper for MultiServerMC

License:        ASL 2.0
URL:            https://multimc.org
BuildArch:      x86_64

Requires:       zenity qt5-qtbase wget
Provides:       multiservermc MultiServerMC multiservermc5

%description
A local install wrapper for MultiServerMC

%prep


%build


%install
mkdir -p %{buildroot}/opt/multiservermc
install -m 0644 ../ubuntu/multiservermc/opt/multiservermc/icon.svg %{buildroot}/opt/multiservermc/icon.svg
install -m 0755 ../ubuntu/multiservermc/opt/multiservermc/run.sh %{buildroot}/opt/multiservermc/run.sh
mkdir -p %{buildroot}/%{_datadir}/applications
install -m 0644 ../ubuntu/multiservermc/usr/share/applications/multiservermc.desktop %{buildroot}/%{_datadir}/applications/multiservermc.desktop
mkdir -p %{buildroot}/%{_metainfodir}
install -m 0644 ../ubuntu/multiservermc/usr/share/metainfo/multiservermc.metainfo.xml %{buildroot}/%{_metainfodir}/multiservermc.metainfo.xml

%files
%dir /opt/multiservermc
/opt/multiservermc/icon.svg
/opt/multiservermc/run.sh
%{_datadir}/applications/multiservermc.desktop
%{_metainfodir}/multiservermc.metainfo.xml


%changelog

* Tue Dec 08 00:34:35 CET 2020 joshua-stone <joshua.gage.stone@gmail.com>
- Add metainfo.xml for improving package metadata

* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
