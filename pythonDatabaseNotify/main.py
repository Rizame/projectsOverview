import firebase_admin
import asyncio
from typing import Final
from telegram import Update
from telegram.ext import Application, CommandHandler, CallbackContext
from firebase_admin import db, credentials

cred = credentials.Certificate('jsonPath')  
firebase_admin.initialize_app(cred, {'databaseURL': 'https://safer-saxion-default-rtdb.europe-west1.firebasedatabase.app/'})
TOKEN: Final = 'BOT_TOKEN_HERE'
BOT_USERNAME: Final = '@safer_vault_bot'


oldFailAttempts = 0
connection = True
statusLast = None
status = None
oldlocker = None



async def stop(update: Update, context: CallbackContext) -> None:
    await update.message.reply_text('Bye!')
    app.stop_running()

async def poll_locker(context: CallbackContext):
    global status
    global oldlocker
    global oldFailAttempts

    failRef = db.reference('/SAFER_STATUS/set_realtime_status/fail_attempt')
    failAttempts = failRef.get()
    statusref = db.reference('/SAFER_STATUS/set_realtime_status/connection_status_seed')
    status = statusref.get()
    lockerRef = db.reference('/SAFER_STATUS/set_realtime_status/open_or_closed')
    locker = lockerRef.get()

    if failAttempts != oldFailAttempts:
        if failAttempts >= 0:
            if failAttempts == 0:
                None
            else:
                await context.job.data.message.reply_text(f'There was an attempt to open the vault, {3 - failAttempts} left.')
            if failAttempts == 3:
                await context.job.data.message.reply_text(f' Starting alarm protocol...')

    if locker != oldlocker:
        for i in range(3):
            await context.job.data.message.reply_text(f'The vault has been {locker}!')
            await asyncio.sleep(2)
    
    oldFailAttempts = failAttempts
    oldlocker = locker


async def poll_status(context: CallbackContext):
    global status
    global statusLast
    global connection
    
    if status == statusLast:
        connection = False
        for i in range(2):
            await context.job.data.message.reply_text('Connection with arduino interrupted')
            await asyncio.sleep(3)
    elif status != statusLast and connection == False:
        connection = True
        for i in range(3):
            await context.job.data.message.reply_text('Connection with arduino established')
            await asyncio.sleep(2)
    
    statusLast = status    


async def start(update: Update, context: CallbackContext) -> None:
    ref = db.reference('/SAFER_STATUS/set_realtime_status/open_or_closed')
    locker = ref.get()
    await update.message.reply_text("Hello, this is a 'Safer' telegram bot. I will provide you with notifications about your vault.")
    await update.message.reply_text(f'Current locker value: {locker}')
    global oldlocker
    oldlocker = locker
    context.job_queue.run_repeating(poll_locker, interval=3, data=update)
    context.job_queue.run_repeating(poll_status, interval=10, data=update)



if __name__ == '__main__':
    app = Application.builder().token(TOKEN).build()
    app.add_handler(CommandHandler('start', start))
    app.add_handler(CommandHandler('stop', stop))
    print("Polling...")
    app.run_polling(poll_interval=3)    