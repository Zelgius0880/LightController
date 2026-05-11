package com.zelgius.lightcontroller.ui.settings

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.twotone.ArrowBack
import androidx.compose.material.icons.twotone.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.zelgius.lightcontroller.selfHosted
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.dialogs.FileKitType
import io.github.vinceglb.filekit.dialogs.openFilePicker
import kotlinx.coroutines.launch
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.back
import lightcontroller.composeapp.generated.resources.connected
import lightcontroller.composeapp.generated.resources.connecting
import lightcontroller.composeapp.generated.resources.create_netatmo_token
import lightcontroller.composeapp.generated.resources.database_backup
import lightcontroller.composeapp.generated.resources.disconnected
import lightcontroller.composeapp.generated.resources.export
import lightcontroller.composeapp.generated.resources.import
import lightcontroller.composeapp.generated.resources.netatmo_token
import lightcontroller.composeapp.generated.resources.server_ip_label
import lightcontroller.composeapp.generated.resources.server_port_label
import lightcontroller.composeapp.generated.resources.server_settings
import lightcontroller.composeapp.generated.resources.settings
import lightcontroller.composeapp.generated.resources.test_connection_and_save
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel

@Composable
fun SettingsScreen(
    viewModel: SettingsViewModel = koinViewModel(),
    onBack: (() -> Unit)? = null
) {
    val state by viewModel.state.collectAsState()

    val snackbarHostState = remember { SnackbarHostState() }

    val snackbarMessage by viewModel.snackbarMessage.collectAsState()

    // Listen for events from ViewModel
    LaunchedEffect(snackbarMessage) {
        snackbarMessage?.let { message ->
            snackbarHostState.showSnackbar(message)
            viewModel.clearMessage()
        }
    }

    val coroutineScope = rememberCoroutineScope()

    Settings(
        snackbarHostState = snackbarHostState,
        state = state,
        onBack = onBack,
        onIpChanged = viewModel::onIpChanged,
        onPortChanged = viewModel::onPortChanged,
        onTryConnection = viewModel::testConnection,
        onExport = viewModel::onExport,
        onImport = {
            coroutineScope.launch {
                val file = FileKit.openFilePicker(type = FileKitType.File("db"))
                if(file != null) viewModel.onImport(file)
            }
        },
        onNetatmoToken = viewModel::openNetatmoTokenTab
    )
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@Composable
private fun Settings(
    state: SettingState,
    snackbarHostState: SnackbarHostState = remember { SnackbarHostState() },
    onIpChanged: (String) -> Unit = {},
    onPortChanged: (String) -> Unit = {},
    onTryConnection: () -> Unit = {},
    onImport: () -> Unit = {},
    onExport: () -> Unit = {},
    onNetatmoToken: () -> Unit = {},
    onBack: (() -> Unit)? = null
) {

    Scaffold(
        snackbarHost = { SnackbarHost(hostState = snackbarHostState) },
        topBar = {
            TopAppBar(
                title = { Text(stringResource(Res.string.settings)) },
                navigationIcon = {
                    if (onBack != null) {
                        IconButton(onClick = onBack) {
                            Icon(
                                Icons.AutoMirrored.TwoTone.ArrowBack,
                                contentDescription = stringResource(Res.string.back)
                            )
                        }
                    }
                }
            )
        }
    ) {
        when (state) {
            is SettingState.Loading -> Box(modifier = Modifier.padding(it).fillMaxSize()) {
                CircularWavyProgressIndicator(modifier = Modifier.align(Alignment.Center))
            }

            is SettingState.Loaded -> Content(
                modifier = Modifier.padding(it),
                onTryConnection = onTryConnection,
                onPortChanged = onPortChanged,
                onIpChanged = onIpChanged,
                state = state,
                onImport = onImport,
                onExport = onExport,
                onNetatmoToken = onNetatmoToken
            )
        }
    }
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
private fun Content(
    state: SettingState.Loaded,
    onIpChanged: (String) -> Unit,
    onPortChanged: (String) -> Unit,
    onTryConnection: () -> Unit,
    onImport: () -> Unit,
    onExport: () -> Unit,
    onNetatmoToken: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {

        Text(
            text = stringResource(Res.string.server_settings),
            style = MaterialTheme.typography.headlineMedium
        )

        OutlinedTextField(
            shape = CircleShape,
            value = state.ip ?: "",
            onValueChange = onIpChanged,
            label = { Text(stringResource(Res.string.server_ip_label)) },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true
        )

        OutlinedTextField(
            shape = CircleShape,
            value = "${state.port}",
            onValueChange = onPortChanged,
            label = { Text(stringResource(Res.string.server_port_label)) },
            modifier = Modifier.fillMaxWidth(),
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
            singleLine = true
        )

        val connected = state.connected

        Button(
            onClick = onTryConnection,
            enabled = connected != null,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(stringResource(Res.string.test_connection_and_save))
        }

        if (!selfHosted) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {

                if (connected != null) {
                    val statusColor =
                        if (state.connected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.error
                    Surface(
                        modifier = Modifier.size(12.dp),
                        shape = MaterialTheme.shapes.small,
                        color = statusColor
                    ) {}

                    Text(
                        text = stringResource(if (state.connected) Res.string.connected else Res.string.disconnected),
                        style = MaterialTheme.typography.bodyMedium
                    )
                } else {
                    CircularWavyProgressIndicator(modifier = Modifier.size(32.dp))

                    Text(stringResource(Res.string.connecting))
                }
            }
        }

        Text(
            text = stringResource(Res.string.database_backup),
            style = MaterialTheme.typography.headlineMedium
        )

        Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
            Button(
                onClick = onImport
            ) {
                Text(stringResource(Res.string.import))
            }
            Button(
                onClick = onExport
            ) {
                Text(stringResource(Res.string.export))
            }
        }

        if( selfHosted) {
            Text(
                text = stringResource(Res.string.netatmo_token),
                style = MaterialTheme.typography.headlineMedium
            )

            Button(
                onClick = onNetatmoToken
            ) {
                Text(stringResource(Res.string.create_netatmo_token))
            }
        }


    }
}

@Preview
@Composable
fun Settings_Loading() {
    Settings(state = SettingState.Loading)
}

@Preview
@Composable
fun Settings_Loaded() {
    Settings(state = SettingState.Loaded(connected = null))
}
