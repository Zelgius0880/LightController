package com.zelgius.lightcontroller.domain.web

import io.ktor.client.HttpClient
import io.ktor.client.plugins.websocket.webSocket
import io.ktor.http.HttpMethod
import io.ktor.websocket.Frame
import io.ktor.websocket.readText
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.plus
import org.koin.core.annotation.Named
import org.koin.core.annotation.Singleton


@Singleton
class WebSocketRepository(
    @Named("WebSocket") private val client: HttpClient
) {
    private val _wsConnectedFlow = MutableStateFlow(false)
    val wsConnectedFlow get() = _wsConnectedFlow.asStateFlow()

    private val _wsMessageFlow = MutableSharedFlow<WebSocketMessage>()
    val wsMessageFlow get() = _wsMessageFlow.asSharedFlow()


    private var connectionJob: Job? = null

    private var currentIp: String? = null
    private var currentPort: Int? = null

    fun connectWs(ip: String, port: Int, onConnection: (connected: Boolean) -> Unit = {}) {
        if(currentPort != port || currentIp != ip) {
            disconnect()

            connectionJob = CoroutineScope(Dispatchers.Default + SupervisorJob()).launch {
                try {
                    client.webSocket(
                        method = HttpMethod.Get,
                        host = ip,
                        port = port,
                    ) {
                        _wsConnectedFlow.value = true
                        onConnection(true)

                        currentPort = port
                        currentIp = ip

                        try {
                            for (frame in incoming) {
                                if (frame is Frame.Text) {
                                    _wsMessageFlow.tryEmit(WebSocketMessage.fromString(frame.readText()))
                                }
                            }
                        } finally {
                            _wsConnectedFlow.value = false
                        }
                    }
                } catch (e: Exception) {
                    onConnection(false)
                    println("Connection error: ${e.message}")
                    _wsConnectedFlow.value = false
                }
            }
        } else onConnection(_wsConnectedFlow.value)
    }

    fun disconnect() {
        connectionJob?.cancel()
        connectionJob = null
        _wsConnectedFlow.value = false

        currentPort = null
        currentIp = null
    }
}