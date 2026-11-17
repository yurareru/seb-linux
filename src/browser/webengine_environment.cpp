#include "webengine_environment.h"

#include "webengine_compat.h"

#include <QStringList>

#include <algorithm>

namespace seb::browser {
namespace {

QString buildProxyServerValue(const seb::settings::browser::ProxySettings &proxySettings)
{
    QStringList parts;
    for (const auto &proxy : proxySettings.Proxies) {
        QString protocol;
        QString address = QStringLiteral("%1:%2").arg(proxy.Host).arg(proxy.Port);

        switch (proxy.Protocol) {
        case seb::settings::browser::proxy::ProxyProtocol::Ftp:
            protocol = QStringLiteral("ftp");
            break;
        case seb::settings::browser::proxy::ProxyProtocol::Http:
            protocol = QStringLiteral("http");
            break;
        case seb::settings::browser::proxy::ProxyProtocol::Https:
            protocol = QStringLiteral("https");
            break;
        case seb::settings::browser::proxy::ProxyProtocol::Socks:
            protocol = QStringLiteral("socks5");
            address = QStringLiteral("socks5://%1").arg(address);
            break;
        }

        parts << QStringLiteral("%1=%2").arg(protocol, address);
    }

    return parts.join(';');
}

}  // namespace

void applyWebEngineEnvironment(const seb::settings::browser::BrowserSettings &settings)
{
#if !SEB_HAS_QTWEBENGINE
    Q_UNUSED(settings);
    return;
#else
    if (settings.Proxy.Policy != seb::settings::browser::proxy::ProxyPolicy::Custom) {
        return;
    }

    QStringList flags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS").split(' ', Qt::SkipEmptyParts);

    if (settings.Proxy.AutoDetect) {
        flags << QStringLiteral("--proxy-auto-detect");
    } else if (settings.Proxy.AutoConfigure && !settings.Proxy.AutoConfigureUrl.isEmpty()) {
        flags << QStringLiteral("--proxy-pac-url=%1").arg(settings.Proxy.AutoConfigureUrl);
    } else {
        const QString proxyServer = buildProxyServerValue(settings.Proxy);
        if (!proxyServer.isEmpty()) {
            flags << QStringLiteral("--proxy-server=%1").arg(proxyServer);
        }
    }

    if (!settings.Proxy.BypassList.isEmpty()) {
        flags << QStringLiteral("--proxy-bypass-list=%1").arg(settings.Proxy.BypassList.join(';'));
    }

    flags.removeDuplicates();
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.join(' ').toUtf8());
#endif
}

void applyMediaCaptureEnvironment()
{
#if SEB_HAS_QTWEBENGINE
    // Official SEB starts CEF with enable-usermedia-screen-capturing and use-fake-ui-for-media-stream,
    // so web pages (e.g. proctoring) get camera, microphone and screen capture without a picker.
    QStringList flags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS").split(' ', Qt::SkipEmptyParts);
    flags << QStringLiteral("--enable-usermedia-screen-capturing")
          << QStringLiteral("--use-fake-ui-for-media-stream");

    // On Wayland, Chromium can only capture the screen through PipeWire and the screen-cast portal.
    if (qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland") ||
        !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        const QString feature = QStringLiteral("WebRTCPipeWireCapturer");
        const QString prefix = QStringLiteral("--enable-features=");
        auto existing = std::find_if(flags.begin(), flags.end(), [&prefix](const QString &flag) {
            return flag.startsWith(prefix);
        });
        if (existing == flags.end()) {
            flags << prefix + feature;
        } else if (!existing->mid(prefix.size()).split(',').contains(feature)) {
            existing->append(QLatin1Char(',') + feature);
        }
    }

    flags.removeDuplicates();
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.join(' ').toUtf8());
#endif
}

}  // namespace seb::browser
