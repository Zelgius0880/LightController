package com.zelgius.lightcontroller

import android.app.Application
import android.content.Context
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.WorkManager.Companion.getInstance
import java.time.Duration
import java.time.LocalDateTime
import java.time.LocalTime
import java.util.concurrent.TimeUnit

class LightControllerApplication : Application() {

    override fun onCreate() {
        super.onCreate()

        scheduleDailyUpload(this)
    }

    fun calculateInitialDelay(): Long {
        val now = LocalDateTime.now()
        val targetTime = LocalTime.of(3, 0) // 3 AM
        var nextRun = now.with(targetTime)

        // If it's already past 3 AM today, schedule for 3 AM tomorrow
        if (now.isAfter(nextRun)) {
            nextRun = nextRun.plusDays(1)
        }

        return Duration.between(now, nextRun).toMillis()
    }

    fun scheduleDailyUpload(context: Context) {
        val delay = calculateInitialDelay()

        val constraints = Constraints.Builder()
            .setRequiredNetworkType(NetworkType.CONNECTED)
            .build()

        val uploadWorkRequest = PeriodicWorkRequestBuilder<DailyUploadWorker>(1, TimeUnit.DAYS)
            .setInitialDelay(delay, TimeUnit.MILLISECONDS)
            .setConstraints(constraints)
            .build()

        getInstance(context).enqueueUniquePeriodicWork(
            "Daily3AMUpload",
            ExistingPeriodicWorkPolicy.UPDATE, // UPDATE ensures the delay is recalculated if the app updates
            uploadWorkRequest
        )
    }
}