#include "BrowserHandler.h"
#include "RenderHandler.h"

#include "include\cef_app.h"
#include "include\wrapper\cef_helpers.h"

BrowserHandler::BrowserHandler(const PDFParameters parameters)
{
    m_parameters = parameters;
}

// CefClient methods:
// -------------------------------------------------------------------------
CefRefPtr<CefLifeSpanHandler> BrowserHandler::GetLifeSpanHandler()
{
    return this;
}

CefRefPtr<CefLoadHandler> BrowserHandler::GetLoadHandler()
{
    return this;
}

CefRefPtr<CefRenderHandler> BrowserHandler::GetRenderHandler()
{
    return new RenderHandler;
}

// CefLifeSpanHandler methods:
// -------------------------------------------------------------------------
void BrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    DLOG(INFO) << "BrowserHandler::OnAfterCreated";

    CEF_REQUIRE_UI_THREAD();

    if (!m_browser.get()) {
        m_browser = browser;

        // set default string encoding to UTF-8
        CefRefPtr<CefValue> value = CefValue::Create();
        value->SetString("utf-8");

        CefString error;

        CefRequestContext::GetGlobalContext()
            ->SetPreference("intl.charset_default", value, error);
    }

    m_browserCount++;
}

bool BrowserHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    DLOG(INFO) << "BrowserHandler::DoClose";

    CEF_REQUIRE_UI_THREAD();

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void BrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    DLOG(INFO) << "BrowserHandler::OnBeforeClose";

    CEF_REQUIRE_UI_THREAD();
    DCHECK(m_browser.get());

    if (browser->IsSame(m_browser)) {
        // Free the browser pointer so that the browser can be destroyed.
        m_browser = NULL;
    }

    if (--m_browserCount == 0) {
        // All browser windows have closed. Quit the application message loop.
        CefQuitMessageLoop();
    }
}

// CefPdfPrintCallback methods:
// -------------------------------------------------------------------------
void BrowserHandler::OnPdfPrintFinished(const CefString& path, bool ok)
{
    DLOG(INFO) << "BrowserHandler::OnPdfPrintFinished";

    CEF_REQUIRE_UI_THREAD();

    m_browser->GetHost()->CloseBrowser(true);
}

// CefLoadHandler methods:
// -------------------------------------------------------------------------
void BrowserHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    DLOG(INFO) << "BrowserHandler::OnLoadEnd";

    CEF_REQUIRE_UI_THREAD();

    if (frame->GetURL() == m_parameters.url) {
        PrintHandler::PaperSize paperSize = PrintHandler::paperSizes[m_parameters.paperSize];

        CefPdfPrintSettings pdfSettings;
        pdfSettings.backgrounds_enabled = true;
        pdfSettings.page_width = paperSize.width;
        pdfSettings.page_height = paperSize.height;
        pdfSettings.landscape = m_parameters.landscape;

        // Save page to file
        m_browser->GetHost()->PrintToPDF(m_parameters.output, pdfSettings, this);
    } else {
        // Load page from URL
        m_browser->GetMainFrame()->LoadURL(m_parameters.url);
    }
}

void BrowserHandler::OnLoadError(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    ErrorCode errorCode,
    const CefString& errorText,
    const CefString& failedUrl
) {
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED) {
        return;
    }

    // TODO: Display a load error message.
}
