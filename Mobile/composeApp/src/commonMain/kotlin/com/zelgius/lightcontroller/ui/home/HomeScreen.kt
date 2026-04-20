package com.zelgius.lightcontroller.ui.home

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import com.zelgius.lightcontroller.navigation.Route
import com.zelgius.lightcontroller.navigation.Settings
import org.koin.compose.viewmodel.koinViewModel

@Composable
fun HomeScreen(
    viewModel: HomeViewModel = koinViewModel(),
    onRouteChanged: (Route) -> Unit
) {
    LaunchedEffect(Unit) {
        viewModel.init()
    }

    val state by viewModel.state.collectAsState()

    LaunchedEffect((state as? HomeState.Loaded)?.settingsSet) {
        if ((state as? HomeState.Loaded)?.settingsSet == false) onRouteChanged(Settings)
    }

    Home(state = state)
}

@Composable
private fun Home(
    state: HomeState
) {

    Box(
        modifier = Modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                text = "Home Screen",
                style = MaterialTheme.typography.headlineMedium
            )
        }
    }
}