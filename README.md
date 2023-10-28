# librime-predict
librime plugin. predict next word.

## Usage
* Put `predict.db` in rime user directory.
* In `*.schema.yaml`, add `predictor` to the list of `engine/processors` before `key_binder`. or patch the schema with: `engine/processors/@before 0: predictor`
* 3 parameters for customizing your predictor:
```yaml
engine/predictor/db: predict.db         # predict db file in user directory/shared directory
                                        # if no valid file found, fallback to 'predict.db'
engine/predictor/max_candidates: 5      # max prediction candidates every time
                                        # if no valid value set, it will show all candidates.
engine/predictor/max_iteration: 5       # max continuely prediction times
                                        # if no valid value set, it will be 0, then no limitation on this.
```
* Deploy and enjoy.
