package com.zelgius.lightcontroller

import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.window.ComposeViewport
import androidx.navigation.ExperimentalBrowserHistoryApi
import com.zelgius.lightcontroller.di.initKoin
import kotlinx.browser.document
import org.jetbrains.compose.resources.ExperimentalResourceApi
import org.jetbrains.compose.resources.configureWebResources

@OptIn(ExperimentalComposeUiApi::class, ExperimentalBrowserHistoryApi::class)
fun main() {
    initKoin{
        modules()
    }
    val body = document.body ?: return

    @OptIn(ExperimentalResourceApi::class)
    configureWebResources {
        resourcePathMapping  { path -> "https://cdn.jsdelivr.net/gh/Zelgius0880/LightController@main/static/$path" }
    }

    ComposeViewport {
        App()
    }
}
