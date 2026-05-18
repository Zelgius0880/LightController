package com.zelgius.lightcontroller.di

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import com.zelgius.lightcontroller.Factory
import com.zelgius.lightcontroller.PLATFORM
import com.zelgius.lightcontroller.Platform
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepository
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepositoryImpl
import com.zelgius.lightcontroller.platformHttpClient
import io.ktor.client.HttpClient
import io.ktor.client.plugins.contentnegotiation.ContentNegotiation
import io.ktor.client.plugins.websocket.WebSockets
import io.ktor.client.request.HttpRequestPipeline
import io.ktor.http.ContentType
import io.ktor.http.URLProtocol
import io.ktor.http.encodedPath
import io.ktor.serialization.kotlinx.KotlinxSerializationConverter
import io.ktor.serialization.kotlinx.json.json
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import kotlinx.serialization.json.Json
import org.koin.core.annotation.ComponentScan
import org.koin.core.annotation.Module
import org.koin.core.annotation.Named
import org.koin.core.annotation.Provided
import org.koin.core.annotation.Single

@Module
@ComponentScan(
    "com.zelgius.lightcontroller.data",
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


    @Single
    @Named("REST")
    fun provideHttpClient(settingsRepository: SettingsRepository) = HttpClient {
        install(ContentNegotiation) {
            json(Json {
                ignoreUnknownKeys = true
                prettyPrint = true
            })
        }
    }

    @Single
    fun provideSettingRepository(dataStore: DataStore<Preferences>): SettingsRepository =
        SettingsRepositoryImpl(dataStore)


    @Single
    fun provideStoredImageRepository(@Provided factory: Factory): StoredImageRepository =
        factory.createStoredImageRepository()

    @Single
    @Named("Proxy")
    fun provideProxyHttpClient(settingsRepository: SettingsRepository) =
        platformHttpClient {
            install(ContentNegotiation) {
                json(Json {
                    ignoreUnknownKeys = true
                    prettyPrint = true
                })
            }

            if (PLATFORM == Platform.Web) {
                install("ProxyInterceptor") {
                    requestPipeline.intercept(HttpRequestPipeline.Render) {
                        // 1. Fetch current IP from your DataStore/Repository
                        // (Assuming you have a way to get this synchronously or via runBlocking)
                        val proxyIp = settingsRepository.ipFlow.filterNotNull().first()

                        if (proxyIp.isNotBlank()) {
                            val originalUrl = context.url.buildString()

                            // 2. Rewrite the request to target the ESP32
                            context.url.apply {
                                protocol = URLProtocol.HTTP
                                host = proxyIp
                                port = 80 // or your ESP32 port

                                // Move the entire original URL into a single query param
                                encodedPath = "/proxy"
                                parameters.clear()
                                parameters.append("url", originalUrl)
                            }
                        }
                    }

                }

            }
        }
}