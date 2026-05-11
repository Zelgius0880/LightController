package com.zelgius.lightcontroller

import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.window.ComposeViewport
import androidx.navigation.ExperimentalBrowserHistoryApi
import androidx.navigation.bindToBrowserNavigation
import com.zelgius.lightcontroller.di.initKoin
import kotlinx.browser.document
import org.koin.core.module.dsl.singleOf
import org.koin.dsl.module

@OptIn(ExperimentalComposeUiApi::class, ExperimentalBrowserHistoryApi::class)
fun main() {
    initKoin{
        modules()
    }
    val body = document.body ?: return

    ComposeViewport {
        App()
    }
}
