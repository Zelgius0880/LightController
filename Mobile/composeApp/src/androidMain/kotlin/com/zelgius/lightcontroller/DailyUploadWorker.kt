package com.zelgius.lightcontroller

import android.content.Context
import androidx.work.CoroutineWorker
import androidx.work.WorkerParameters
import com.google.firebase.crashlytics.FirebaseCrashlytics
import com.zelgius.lightcontroller.data.ServerService
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepositoryImpl
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import io.ktor.client.plugins.HttpTimeout
import io.ktor.client.plugins.contentnegotiation.ContentNegotiation
import io.ktor.serialization.kotlinx.json.json
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json

class DailyUploadWorker(
    appContext: Context,
    workerParams: WorkerParameters
) : CoroutineWorker(appContext, workerParams) {

    private val serverRepository = ServerRepository(
        serverService = ServerService(platformHttpClient {
            install(ContentNegotiation) {
                json(Json {
                    ignoreUnknownKeys = true
                    prettyPrint = true
                })
            }

            install(HttpTimeout) {
                requestTimeoutMillis = 10 * 60 * 1000 // 10 minutes
                connectTimeoutMillis = 30_000        // 30 seconds
                socketTimeoutMillis = 60_000         // 1 minute of inactivity allowed
            }
        }),
        settingsRepository = SettingsRepositoryImpl(Factory(appContext).createDataStore())
    )

    private val storedImageRepository = StoredImageRepository(appContext)

    override suspend fun doWork(): Result = withContext(Dispatchers.IO) {
        try {
            val success = performUpload()

            if (success) {
                Result.success()
            } else {
                Result.retry()
            }
        } catch (e: Exception) {
            FirebaseCrashlytics.getInstance().recordException(e)
            Result.retry()
        }
    }

    private suspend fun performUpload(): Boolean {
        val image = storedImageRepository.getThumbnails().random().let {
            storedImageRepository.restoreFullImage(it.id)
        }?: run {
            FirebaseCrashlytics.getInstance().recordException(Exception("Not thumbnails found"))
            return false
        }

        serverRepository.uploadImage(image.pixels)

        return true
    }
}