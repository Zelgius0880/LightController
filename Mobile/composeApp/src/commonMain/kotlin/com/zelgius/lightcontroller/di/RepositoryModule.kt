package com.zelgius.lightcontroller.di

import com.zelgius.lightcontroller.Factory
import io.ktor.client.HttpClient
import io.ktor.client.plugins.websocket.WebSockets
import org.koin.core.annotation.ComponentScan
import org.koin.core.annotation.Module
import org.koin.core.annotation.Named
import org.koin.core.annotation.Provided
import org.koin.core.annotation.Single

@Module
@ComponentScan(
    "com.zelgius.lightcontroller.domain",

)
class RepositoryModule {
    @Single
    @Named("WebSocket")
    fun provideWebSocketHttpClient() = HttpClient {
        install(WebSockets)
    }


    @Single
    fun provideDataStore(@Provided factory: Factory) = factory.createDataStore()

}