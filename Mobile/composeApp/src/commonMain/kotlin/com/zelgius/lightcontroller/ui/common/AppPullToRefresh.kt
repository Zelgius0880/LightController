package com.zelgius.lightcontroller.ui.common

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.twotone.Refresh
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LoadingIndicator
import androidx.compose.material3.pulltorefresh.PullToRefreshBox
import androidx.compose.material3.pulltorefresh.PullToRefreshDefaults
import androidx.compose.material3.pulltorefresh.rememberPullToRefreshState
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.zelgius.lightcontroller.PLATFORM
import com.zelgius.lightcontroller.Platform

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun AppPullToRefresh(
    onRefresh: () -> Unit,
    modifier: Modifier = Modifier,
    isRefreshing: Boolean,
    content: @Composable BoxScope.() -> Unit
) {
    if(PLATFORM != Platform.Web) {
        val pullToRefreshState = rememberPullToRefreshState()
        PullToRefreshBox(
            modifier = modifier.fillMaxSize(),
            isRefreshing = isRefreshing,
            state = pullToRefreshState,
            onRefresh = onRefresh,
            indicator = {
                PullToRefreshDefaults.LoadingIndicator(
                    modifier = Modifier.align(Alignment.TopCenter),
                    state = pullToRefreshState,
                    isRefreshing = isRefreshing,
                )
            },
            content = content
        )
    } else  {
        Box(modifier.fillMaxSize()) {

            Box(Modifier.fillMaxSize().padding(top = 36.dp)) {
                content()
            }

            AnimatedVisibility(isRefreshing, modifier = Modifier.align(Alignment.TopCenter)) {
                LoadingIndicator()
            }

            IconButton(onRefresh, modifier= Modifier.align(Alignment.TopEnd)) {
                Icon(Icons.TwoTone.Refresh, contentDescription =  "Refresh")
            }
        }
    }
}